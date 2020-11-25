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
    uint32_t primary_addr, uint32_t secondary_addr)
    : RadioInterface(ce_pin, tunnel_fd, primary_addr, secondary_addr) {
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
    std::lock_guard<std::mutex> lock(read_buffer_mutex_);

    Request request;
    auto* tunnel = request.mutable_network_tunnel_txrx();
    size_t transfer_size = std::min(read_buffer_.size(),
        static_cast<size_t>(16));
    tunnel->set_payload({read_buffer_.begin(), read_buffer_.begin()
        + transfer_size});

    auto result = Send(request);
    if (result != RequestResult::Success) {
      LOGE("Failed to send network tunnel txrx request");
      continue;
    }

    SleepUs(5000);

    Response response;
    result = Receive(response, /*timeout_us=*/100000);
    if (result != RequestResult::Success) {
      LOGE("Failed to receive network tunnel txrx request");
      continue;
    }

    read_buffer_.erase(read_buffer_.begin(),
        read_buffer_.begin() + transfer_size);

    // TODO: Check that the response is well formed.

    int bytes_written = write(tunnel_fd_,
        response.network_tunnel_txrx().payload().data(),
        response.network_tunnel_txrx().payload().size());
    LOGI("Wrote %d bytes from the tunnel", bytes_written);
  }
}

}  // namespace nerfnet
