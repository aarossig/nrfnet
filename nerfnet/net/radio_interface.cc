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

#include "nerfnet/net/radio_interface.h"

#include "nerfnet/util/log.h"
#include "nerfnet/util/time.h"

namespace nerfnet {

RadioInterface::RadioInterface(uint16_t ce_pin, int tunnel_fd,
                               uint32_t primary_addr, uint32_t secondary_addr)
    : radio_(ce_pin, 0),
      tunnel_fd_(tunnel_fd),
      primary_addr_(primary_addr),
      secondary_addr_(secondary_addr) {
  radio_.begin();
  radio_.setChannel(1);
  radio_.setPALevel(RF24_PA_MAX);
  radio_.setDataRate(RF24_1MBPS);
  radio_.setAutoAck(1);
  radio_.setRetries(2, 15);
  radio_.setCRCLength(RF24_CRC_8);
  CHECK(radio_.isChipConnected(), "NRF24L01 is unavailable");
}

RadioInterface::RequestResult RadioInterface::Send(
    const google::protobuf::Message& request) {
  radio_.stopListening();

  std::string serialized_request;
  CHECK(request.SerializeToString(&serialized_request),
      "failed to encode message");
  if (serialized_request.size() > (kMaxPacketSize - 1)) {
    LOGE("serialized message is too large (%zu vs %zu)",
        serialized_request.size(), kMaxPacketSize);
    return RequestResult::Malformed;
  }

  LOGI("Sending packet with length: %zu", serialized_request.size());
  for (size_t i = 0; i < serialized_request.size(); i++) {
    LOGI("Sending byte %zu=%02x", i, serialized_request[i]);
  }

  serialized_request.insert(serialized_request.begin(),
      static_cast<char>(serialized_request.size()));
  if (!radio_.write(serialized_request.data(), serialized_request.size())) {
    LOGE("failed to write request");
    return RequestResult::TransmitError;
  }

  radio_.startListening();
  return RequestResult::Success;
}

RadioInterface::RequestResult RadioInterface::Receive(
    google::protobuf::Message& response, uint64_t timeout_us) {
  uint64_t start_us = TimeNowUs();

  while (!radio_.available()) {
    if (timeout_us != 0 && (start_us + timeout_us) < TimeNowUs()) {
      LOGE("Timeout receiving response");
      return RequestResult::Timeout;
    }

    SleepUs(1000);
  }

  std::string packet(kMaxPacketSize, '\0');
  radio_.read(packet.data(), packet.size());

  size_t size = packet[0];
  if (size > (packet.size() - 1)) {
    LOGE("Received message too large");
    return RequestResult::Malformed;
  }

  packet = packet.substr(1, size);
  if (!response.ParseFromString(packet)) {
    LOGE("Received message failed to deserialize");
    return RequestResult::Malformed;
  }

  return RequestResult::Success;
}

}  // namespace nerfnet
