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

#include <unistd.h>

#include "nerfnet/util/log.h"
#include "nerfnet/util/time.h"

namespace nerfnet {

RadioInterface::RadioInterface(uint16_t ce_pin, int tunnel_fd,
                               uint32_t primary_addr, uint32_t secondary_addr)
    : radio_(ce_pin, 0),
      tunnel_fd_(tunnel_fd),
      primary_addr_(primary_addr),
      secondary_addr_(secondary_addr),
      tunnel_thread_(&RadioInterface::TunnelThread, this),
      next_id_(1) {
  radio_.begin();
  radio_.setChannel(1);
  radio_.setPALevel(RF24_PA_MAX);
  radio_.setDataRate(RF24_2MBPS);
  radio_.setAddressWidth(3);
  radio_.setAutoAck(1);
  radio_.setRetries(0, 15);
  radio_.setCRCLength(RF24_CRC_16);
  CHECK(radio_.isChipConnected(), "NRF24L01 is unavailable");
}

RadioInterface::~RadioInterface() {
  running_ = false;
  tunnel_thread_.join();
}

RadioInterface::RequestResult RadioInterface::Send(
    const google::protobuf::Message& request) {
  radio_.stopListening();

  std::string serialized_request;
  CHECK(request.SerializeToString(&serialized_request),
      "failed to encode message");
  if (serialized_request.size() > (kMaxPacketSize - 1)) {
    LOGE("serialized message is too large (%zu vs %zu)",
        serialized_request.size(), (kMaxPacketSize - 1));
    radio_.startListening();
    return RequestResult::Malformed;
  }

  serialized_request.insert(serialized_request.begin(),
      static_cast<char>(serialized_request.size()));
  if (!radio_.write(serialized_request.data(), serialized_request.size())) {
    LOGE("Failed to write request");
    radio_.startListening();
    return RequestResult::TransmitError;
  }

  while (!radio_.txStandBy()) {
    LOGI("Waiting for transmit standby");
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

size_t RadioInterface::GetReadBufferSize() {
  std::lock_guard<std::mutex> lock(read_buffer_mutex_);
  return read_buffer_.size();
}

void RadioInterface::AdvanceID() {
  next_id_++;
  if (next_id_ > 0x7) {
    next_id_ = 1;
  }
}

bool RadioInterface::ValidateID(uint8_t id) {
  if (!last_ack_id_.has_value()
      || (last_ack_id_.value() == 0x07 && id == 1)
      || (id == (last_ack_id_.value() + 1))) {
    last_ack_id_ = id;
    return true;
  }

  return false;
}

void RadioInterface::TunnelThread() {
  // The maximum number of network frames to buffer here.
  constexpr size_t kMaxBufferedFrames = 1024;

  running_ = true;
  uint8_t buffer[3200];
  while (running_) {
    int bytes_read = read(tunnel_fd_, buffer, sizeof(buffer));
    if (bytes_read < 0) {
      LOGE("Failed to read: %s (%d)", strerror(errno), errno);
      continue;
    }

    {
      std::lock_guard<std::mutex> lock(read_buffer_mutex_);
      read_buffer_.emplace_back(&buffer[0], &buffer[bytes_read]);
      LOGI("Read %zu bytes from the tunnel", read_buffer_.back().size());
    }

    while (GetReadBufferSize() > kMaxBufferedFrames && running_) {
      SleepUs(1000);
    }
  }
}

}  // namespace nerfnet
