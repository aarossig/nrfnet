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
      next_id_(1),
      tunnel_logs_enabled_(false) {
  CHECK(radio_.begin(), "Failed to start NRF24L01");
  radio_.setChannel(1);
  radio_.setPALevel(RF24_PA_MAX);
  radio_.setDataRate(RF24_2MBPS);
  radio_.setAddressWidth(3);
  radio_.setAutoAck(1);
  radio_.setRetries(0, 15);
  radio_.setCRCLength(RF24_CRC_8);
  CHECK(radio_.isChipConnected(), "NRF24L01 is unavailable");
}

RadioInterface::~RadioInterface() {
  running_ = false;
  tunnel_thread_.join();
}

RadioInterface::RequestResult RadioInterface::Send(
    const std::vector<uint8_t>& request) {
  radio_.stopListening();

  if (request.size() > kMaxPacketSize) {
    LOGE("Request is too large (%zu vs %zu)", request.size(), kMaxPacketSize);
    return RequestResult::Malformed;
  }

  if (!radio_.write(request.data(), request.size())) {
    LOGE("Failed to write request");
    return RequestResult::TransmitError;
  }

  while (!radio_.txStandBy()) {
    LOGI("Waiting for transmit standby");
  }

  return RequestResult::Success;
}

RadioInterface::RequestResult RadioInterface::Receive(
    std::vector<uint8_t>& response, uint64_t timeout_us) {
  radio_.startListening();
  uint64_t start_us = TimeNowUs();
  while (!radio_.available()) {
    if (timeout_us != 0 && (start_us + timeout_us) < TimeNowUs()) {
      LOGE("Timeout receiving response");
      return RequestResult::Timeout;
    }
  }

  radio_.read(response.data(), response.size());
  return RequestResult::Success;
}

size_t RadioInterface::GetReadBufferSize() {
  std::lock_guard<std::mutex> lock(read_buffer_mutex_);
  return read_buffer_.size();
}

size_t RadioInterface::GetTransferSize(const std::vector<uint8_t>& frame) {
  return std::min(frame.size(), static_cast<size_t>(kMaxPayloadSize));
}

void RadioInterface::AdvanceID() {
  next_id_++;
  if (next_id_ > kIDMask) {
    next_id_ = 1;
  }
}

bool RadioInterface::ValidateID(uint8_t id) {
  if (!last_ack_id_.has_value()
      || (last_ack_id_.value() == kIDMask && id == 1)
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
      if (tunnel_logs_enabled_) {
        LOGI("Read %zu bytes from the tunnel", read_buffer_.back().size());
      }
    }

    while (GetReadBufferSize() > kMaxBufferedFrames && running_) {
      SleepUs(1000);
    }
  }
}

bool RadioInterface::DecodeTunnelTxRxPacket(
    const std::vector<uint8_t>& request, TunnelTxRxPacket& tunnel) {
  if (request.size() != kMaxPacketSize) {
    LOGE("Received short TxRx packet");
    return false;
  }

  tunnel.id.reset();
  uint8_t id_value = request[0] & kIDMask;
  if (id_value != 0) {
    tunnel.id = id_value;
  }

  tunnel.ack_id.reset();
  uint8_t ack_id_value = (request[0] >> 4) & kIDMask;
  if (ack_id_value != 0) {
    tunnel.ack_id = ack_id_value;
  }

  tunnel.payload.clear();
  uint8_t size_value = request[1];
  tunnel.bytes_left = size_value;
  if (size_value > 0) {
    size_value = std::min(size_value, static_cast<uint8_t>(kMaxPayloadSize));
    tunnel.payload = {request.begin() + 2, request.begin() + 2 + size_value};
  }

  return true;
}

bool RadioInterface::EncodeTunnelTxRxPacket(
    const TunnelTxRxPacket& tunnel, std::vector<uint8_t>& request) {
  request.resize(kMaxPacketSize, 0x00);
  if (tunnel.id.has_value()) {
    request[0] = tunnel.id.value();
  }

  if (tunnel.ack_id.has_value()) {
    request[0] |= (tunnel.ack_id.value() << 4);
  }

   if (tunnel.payload.size() > kMaxPayloadSize) {
    LOGE("TxRx packet payload is too large");
    return false;
  }

  request[1] = tunnel.bytes_left;
  for (size_t i = 0; i < tunnel.payload.size(); i++) {
    request[2 + i] = tunnel.payload[i];
  }

  return true;
}

void RadioInterface::WriteTunnel() {
  int bytes_written = write(tunnel_fd_,
      frame_buffer_.data(), frame_buffer_.size());
  if (tunnel_logs_enabled_) {
    LOGI("Writing %d bytes to the tunnel", frame_buffer_.size());
  }

  frame_buffer_.clear();
  if (bytes_written < 0) {
    LOGE("Failed to write to tunnel %s (%d)", strerror(errno), errno);
  }
}

}  // namespace nerfnet
