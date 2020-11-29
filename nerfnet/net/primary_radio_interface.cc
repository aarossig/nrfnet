/*
 * Copyright 2020 Andrew Rossignol andrew.rossignol@gmail.com
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nerfnet/net/primary_radio_interface.h"

#include <unistd.h>

#include "nerfnet/util/log.h"
#include "nerfnet/util/macros.h"
#include "nerfnet/util/time.h"

namespace nerfnet {

PrimaryRadioInterface::PrimaryRadioInterface(
    uint16_t ce_pin, int tunnel_fd,
    uint32_t primary_addr, uint32_t secondary_addr, uint8_t channel,
    uint64_t poll_interval_us)
    : RadioInterface(ce_pin, tunnel_fd, primary_addr, secondary_addr, channel),
      poll_interval_us_(poll_interval_us),
      current_poll_interval_us_(poll_interval_us_) {
  uint8_t writing_addr[5] = {
    static_cast<uint8_t>(primary_addr),
    static_cast<uint8_t>(primary_addr >> 8),
    static_cast<uint8_t>(primary_addr >> 16),
    static_cast<uint8_t>(primary_addr >> 24),
    0,
  };

  uint8_t reading_addr[5] = {
    static_cast<uint8_t>(secondary_addr),
    static_cast<uint8_t>(secondary_addr >> 8),
    static_cast<uint8_t>(secondary_addr >> 16),
    static_cast<uint8_t>(secondary_addr >> 24),
    0,
  };

  radio_.openWritingPipe(writing_addr);
  radio_.openReadingPipe(kPipeId, reading_addr);
}

void PrimaryRadioInterface::Run() {
  CHECK(ConnectionReset(), "Failed to open connection");
  while (1) {
    SleepUs(current_poll_interval_us_);
    std::lock_guard<std::mutex> lock(read_buffer_mutex_);
    if (PerformTunnelTransfer()) {
      poll_fail_count_ = 0;
      current_poll_interval_us_ = poll_interval_us_;
    } else {
      poll_fail_count_++;
      if (poll_fail_count_ > 10) {
        if (current_poll_interval_us_ < 1000000) {
          current_poll_interval_us_ *= 2;
        }

        LOGI("Attempting to recover connection");
        if (!ConnectionReset()) {
          LOGE("Connection recovery failed");
        } else {
          LOGI("Connection recovered successfully");
          poll_fail_count_ = 0;
          current_poll_interval_us_ = poll_interval_us_;
        }
      }
    }
  }
}

bool PrimaryRadioInterface::ConnectionReset() {
  next_id_ = 1;
  last_ack_id_.reset();
  frame_buffer_.clear();

  std::vector<uint8_t> request(kMaxPacketSize, 0x00);
  auto result = Send(request);
  if (result != RequestResult::Success) {
    LOGE("Failed to send tunnel reset request");
    return false;
  }

  std::vector<uint8_t> response(kMaxPacketSize, 0x00);
  result = Receive(response, /*timeout_us=*/100000);
  if (result != RequestResult::Success) {
    LOGE("Failed to receive tunnel reset response");
    return false;
  }

  return response[0] == 0x00;
}

bool PrimaryRadioInterface::PerformTunnelTransfer() {
  TunnelTxRxPacket tunnel;
  tunnel.id = next_id_;
  if (last_ack_id_.has_value()) {
    tunnel.ack_id = last_ack_id_.value();
  }

  tunnel.bytes_left = 0;
  if (!read_buffer_.empty()) {
    auto& frame = read_buffer_.front();
    size_t transfer_size = GetTransferSize(frame);
    tunnel.payload = {frame.begin(), frame.begin() + transfer_size};
    tunnel.bytes_left = std::min(frame.size(), static_cast<size_t>(UINT8_MAX));
  }

  std::vector<uint8_t> request;
  CHECK(EncodeTunnelTxRxPacket(tunnel, request),
      "Failed to encode tunnel packet");

  auto result = Send(request);
  if (result != RequestResult::Success) {
    LOGE("Failed to send network tunnel txrx request");
    return false;
  }

  std::vector<uint8_t> response(kMaxPacketSize);
  result = Receive(response, /*timeout_us=*/100000);
  if (result != RequestResult::Success) {
    LOGE("Failed to receive network tunnel txrx request");
    return false;
  }
  
  if (!DecodeTunnelTxRxPacket(response, tunnel)) {
    return false;
  }

  if (!tunnel.id.has_value() || !tunnel.ack_id.has_value()) {
    LOGE("Missing tunnel fields");
    return false;
  }


  bool success = true;
  if (tunnel.ack_id.value() != next_id_) {
    LOGE("Secondary radio failed to ack, retransmitting: "
         "ack_id=%u, next_id=%u", tunnel.ack_id.value(), next_id_);
    success = false;
  } else {
    AdvanceID();
    if (!read_buffer_.empty()) {
      auto& frame = read_buffer_.front();
      frame.erase(frame.begin(), frame.begin() + GetTransferSize(frame));
      if (frame.empty()) {
        read_buffer_.pop_front();
      }
    }
  }

  if (!ValidateID(tunnel.id.value())) {
    LOGE("Received non-sequential packet");
    success = false;
  } else if (!tunnel.payload.empty()) {
    frame_buffer_.insert(frame_buffer_.end(),
        tunnel.payload.begin(), tunnel.payload.end());
    if (tunnel.bytes_left <= kMaxPayloadSize) {
      WriteTunnel();
    }
  }

  return success;
}

}  // namespace nerfnet
