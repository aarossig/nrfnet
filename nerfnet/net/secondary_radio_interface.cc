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

#include "nerfnet/net/secondary_radio_interface.h"

#include <unistd.h>
#include <vector>

#include "nerfnet/util/log.h"
#include "nerfnet/util/time.h"

namespace nerfnet {

SecondaryRadioInterface::SecondaryRadioInterface(
    uint16_t ce_pin, int tunnel_fd,
    uint32_t primary_addr, uint32_t secondary_addr, uint8_t channel)
    : RadioInterface(ce_pin, tunnel_fd, primary_addr, secondary_addr, channel),
      payload_in_flight_(false) {
  uint8_t writing_addr[5] = {
    static_cast<uint8_t>(secondary_addr),
    static_cast<uint8_t>(secondary_addr >> 8),
    static_cast<uint8_t>(secondary_addr >> 16),
    static_cast<uint8_t>(secondary_addr >> 24),
    0,
  };

  uint8_t reading_addr[5] = {
    static_cast<uint8_t>(primary_addr),
    static_cast<uint8_t>(primary_addr >> 8),
    static_cast<uint8_t>(primary_addr >> 16),
    static_cast<uint8_t>(primary_addr >> 24),
    0,
  };

  radio_.openWritingPipe(writing_addr);
  radio_.openReadingPipe(kPipeId, reading_addr);
}

void SecondaryRadioInterface::Run() {
  uint8_t packet[kMaxPacketSize];

  while (1) {
    std::vector<uint8_t> request(kMaxPacketSize, 0x00);
    auto result = Receive(request);
    if (result == RequestResult::Success) {
      HandleRequest(request);
    }
  }
}

void SecondaryRadioInterface::HandleRequest(
    const std::vector<uint8_t>& request) {
  if (request.size() != kMaxPacketSize) {
    LOGE("Received short packet");
  } else if (request[0] == 0x00) {
    HandleNetworkTunnelReset();
  } else {
    HandleNetworkTunnelTxRx(request);
  }
}

void SecondaryRadioInterface::HandleNetworkTunnelReset() {
  next_id_ = 1;
  last_ack_id_.reset();
  frame_buffer_.clear();
  payload_in_flight_ = false;

  LOGI("Responding to tunnel reset request");
  std::vector<uint8_t> response(kMaxPacketSize, 0x00);
  auto status = Send(response);
  if (status != RequestResult::Success) {
    LOGE("Failed to send tunnel reset response");
  }
}

void SecondaryRadioInterface::HandleNetworkTunnelTxRx(
    const std::vector<uint8_t>& request) {
  TunnelTxRxPacket tunnel;
  if (!DecodeTunnelTxRxPacket(request, tunnel)) {
    return;
  }

  std::lock_guard<std::mutex> lock(read_buffer_mutex_);
  if (!tunnel.id.has_value()
      || (last_ack_id_.has_value() && !tunnel.ack_id.has_value())) {
    LOGE("Missing tunnel fields");
    return;
  }

  if (!ValidateID(tunnel.id.value())) {
    LOGE("Received non-sequential packet: %u vs %u",
        last_ack_id_.value(), tunnel.id.value());
  } else if (!tunnel.payload.empty()) {
    frame_buffer_.insert(frame_buffer_.end(),
        tunnel.payload.begin(), tunnel.payload.end());
    if (tunnel.bytes_left <= kMaxPayloadSize) {
      WriteTunnel();
    }
  }

  if (tunnel.ack_id.has_value()) {
    if (tunnel.ack_id.value() != next_id_) {
      LOGE("Primary radio failed to ack, retransmitting");
    } else {
      AdvanceID();
      if (payload_in_flight_) {
        if (!read_buffer_.empty()) {
          auto& frame = read_buffer_.front();
          size_t transfer_size = GetTransferSize(frame);
          frame.erase(frame.begin(), frame.begin() + transfer_size);
          if (frame.empty()) {
            read_buffer_.pop_front();
          }
        }

        payload_in_flight_ = false;
      }
    }
  }

  tunnel.id = next_id_;
  tunnel.ack_id = last_ack_id_.value();
  tunnel.bytes_left = 0;
  if (!read_buffer_.empty()) {
    auto& frame = read_buffer_.front();
    size_t transfer_size = GetTransferSize(frame);
    tunnel.payload = {frame.begin(), frame.begin() + transfer_size};
    tunnel.bytes_left = std::min(frame.size(), static_cast<size_t>(UINT8_MAX));
    payload_in_flight_ = true;
  }

  std::vector<uint8_t> response;
  if (!EncodeTunnelTxRxPacket(tunnel, response)) {
    return;
  }

  auto status = Send(response);
  if (status != RequestResult::Success) {
    LOGE("Failed to send network tunnel txrx response");
  }
}

}  // namespace nerfnet
