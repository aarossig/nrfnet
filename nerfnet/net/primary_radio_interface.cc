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

#include "nerfnet/net/net.pb.h"
#include "nerfnet/util/log.h"
#include "nerfnet/util/macros.h"

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
  radio_.openReadingPipe(1, reading_addr);
}

PrimaryRadioInterface::RequestResult PrimaryRadioInterface::Ping(
    const std::optional<uint32_t>& value) {
  Request request;
  auto ping = request.mutable_ping();
  if (value.has_value()) {
    ping->set_value(*value);
  }

  std::string serialized_message;
  CHECK(request.SerializeToString(&serialized_message),
      "failed to encode ping");
  if (serialized_message.size() > kMaxPacketSize) {
    LOGE("serialized ping is too large (%zu vs %zu)",
        serialized_message.size(), kMaxPacketSize);
    return RequestResult::MalformedRequest;
  }

  LOGI("Sending packet with length: %zu", serialized_message.size());
  for (size_t i = 0; i < serialized_message.size(); i++) {
    LOGI("Sending byte %zu=%02x", i, serialized_message[i]);
  }

  if (!radio_.write(serialized_message.data(), serialized_message.size())) {
    LOGE("failed to write ping request");
    return RequestResult::TransmitError;
  }

  return RequestResult::Success;
}

}  // namespace nerfnet
