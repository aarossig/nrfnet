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
    uint32_t primary_addr, uint32_t secondary_addr, uint64_t rf_delay_us)
    : RadioInterface(ce_pin, tunnel_fd, primary_addr, secondary_addr,
                     rf_delay_us) {
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

PrimaryRadioInterface::RequestResult PrimaryRadioInterface::Ping(
    const std::optional<uint32_t>& value) {
  Request request;
  auto* ping = request.mutable_ping();
  if (value.has_value()) {
    ping->set_value(*value);
  }

  auto result = Send(request);
  if (result != RequestResult::Success) {
    LOGE("Failed to send ping request");
    return result;
  }

  Response response;
  result = Receive(response, /*timeout_us=*/100000);
  if (result != RequestResult::Success) {
    LOGE("Failed to receive ping response");
    return result;
  }

  if (!response.has_ping()) {
    LOGE("Response missing ping response");
    return RequestResult::Malformed;
  } else if (value.has_value()
             && (!response.ping().has_value()
		 || response.ping().value() != value)) {
    LOGE("Response value mismatch");
    return RequestResult::Malformed;
  }

  return result;
}

void PrimaryRadioInterface::Run() {
  while (1) {
    SleepUs(100);

    std::lock_guard<std::mutex> lock(read_buffer_mutex_);

    Request request;
    auto* tunnel_request = request.mutable_network_tunnel_txrx();
    tunnel_request->set_id(next_id_);
    if (last_ack_id_.has_value()) {
      tunnel_request->set_ack_id(last_ack_id_.value());
    }

    size_t transfer_size = 0;
    if (!read_buffer_.empty()) {
      auto& frame = read_buffer_.front();
      transfer_size = std::min(frame.size(), static_cast<size_t>(8));
      tunnel_request->set_payload(
          {frame.begin(), frame.begin() + transfer_size});
      tunnel_request->set_remaining_bytes(frame.size() - transfer_size);
    }

    auto result = Send(request);
    if (result != RequestResult::Success) {
      LOGE("Failed to send network tunnel txrx request");
      continue;
    }

    Response response;
    result = Receive(response, /*timeout_us=*/100000);
    if (result != RequestResult::Success) {
      LOGE("Failed to receive network tunnel txrx request");
      continue;
    }
    
    if (!response.has_network_tunnel_txrx()) {
      LOGE("Missing network tunnel txrx");
      continue;
    }
    
    const auto& tunnel_response = response.network_tunnel_txrx();
    if (!tunnel_response.has_id() || !tunnel_response.has_ack_id()) {
      LOGE("Missing tunnel fields");
      continue;
    }
    
    if (tunnel_response.ack_id() != next_id_) {
      LOGE("Secondary radio failed to ack, retransmitting: "
           "ack_id=%u, next_id=%u", tunnel_response.ack_id(), next_id_);
    } else {
      AdvanceID();
      if (!read_buffer_.empty()) {
        auto& frame = read_buffer_.front();
        frame.erase(frame.begin(), frame.begin() + transfer_size);
        if (frame.empty()) {
          read_buffer_.pop_front();
        }
      }
    }

    if (!ValidateID(tunnel_response.id())) {
      LOGE("Received non-sequential packet");
    } else if (tunnel_response.has_payload()) {
      frame_buffer_ += tunnel_response.payload();
      if (tunnel_response.remaining_bytes() == 0) {
        int bytes_written = write(tunnel_fd_,
            frame_buffer_.data(), frame_buffer_.size());
        LOGI("Writing %d bytes to the tunnel", frame_buffer_.size());
        frame_buffer_.clear();
        if (bytes_written < 0) {
          LOGE("Failed to write to tunnel %s (%d)", strerror(errno), errno);
        }
      }
    }
  }
}

}  // namespace nerfnet
