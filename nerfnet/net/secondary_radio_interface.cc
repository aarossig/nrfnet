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
    uint32_t primary_addr, uint32_t secondary_addr)
    : RadioInterface(ce_pin, tunnel_fd, primary_addr, secondary_addr) {
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
  radio_.startListening();
}

void SecondaryRadioInterface::Run() {
  uint8_t packet[kMaxPacketSize];

  while (1) {
    Request request;
    auto result = Receive(request);
    if (result == RequestResult::Success) {
      HandleRequest(request);
    }
  }
}

void SecondaryRadioInterface::HandleRequest(const Request& request) {
  switch (request.request_case()) {
    case Request::kPing:
      HandlePing(request.ping());
      break;
    case Request::kNetworkTunnelTxrx:
      HandleNetworkTunnelTxRx(request.network_tunnel_txrx());
      break;
    default:
      LOGE("Received unknown request");
      break;
  }
}

void SecondaryRadioInterface::HandlePing(const Request::Ping& ping) {
  Response response;
  auto* ping_response = response.mutable_ping();
  if (ping.has_value()) {
    ping_response->set_value(ping.value());
  }

  auto status = Send(response);
  if (status != RequestResult::Success) {
    LOGE("failed to send ping response");
  }
}

void SecondaryRadioInterface::HandleNetworkTunnelTxRx(
    const Request::NetworkTunnelTxRx& tunnel) {
  std::lock_guard<std::mutex> lock(read_buffer_mutex_);

  Response response;
  auto* tunnel_response = response.mutable_network_tunnel_txrx();
  size_t transfer_size = 0;
  if (!read_buffer_.empty()) {
    auto& frame = read_buffer_.front();
    transfer_size = std::min(frame.size(), static_cast<size_t>(24));
    tunnel_response->set_payload({frame.begin(),
        frame.begin() + transfer_size});
    tunnel_response->set_remaining_bytes(frame.size() - transfer_size);
  }

  SleepUs(5000);

  auto status = Send(response);
  if (status != RequestResult::Success) {
    LOGE("Failed to send network tunnel txrx response");
  } else {
    if (!read_buffer_.empty()) {
      auto& frame = read_buffer_.front();
      frame.erase(frame.begin(), frame.begin() + transfer_size);
      if (frame.empty()) {
        read_buffer_.pop_front();
      }
    }

    if (tunnel.has_payload()) {
      frame_buffer_ += tunnel.payload();
      if (tunnel.remaining_bytes() == 0) {
        int bytes_written = write(tunnel_fd_,
            frame_buffer_.data(), frame_buffer_.size());
        if (bytes_written < 0) {
          LOGE("Failed to write to tunnel %s (%d)", strerror(errno), errno);
        } else {
          LOGI("Wrote %d bytes from the tunnel", bytes_written);
        }

        frame_buffer_.clear();
      }
    }
  }
}

}  // namespace nerfnet
