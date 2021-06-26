/*
 * Copyright 2021 Andrew Rossignol andrew.rossignol@gmail.com
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

#include "nerfnet/net/nrf_link.h"

#include <array>

#include "nerfnet/util/log.h"

namespace nerfnet {

namespace {

// The pipe indexes for broadcast and directed messags.
constexpr uint8_t kBroadcastPipe = 0;
constexpr uint8_t kDirectedPipe = 1;

// The address for broadcast packets. Selected to avoid alternating binary
// as well as many level shifts. See 7.3.2 from the NRF24L01 datasheet for
// further details.
constexpr uint32_t kBroadcastAddress = 0xc341efa2;

// Formats an address to be send to the radio driver.
std::array<uint8_t, 5> FormatAddress(uint32_t address) {
  std::array<uint8_t, 5> buffer;
  buffer[0] = static_cast<uint8_t>(address);
  buffer[1] = static_cast<uint8_t>(address >> 8);
  buffer[2] = static_cast<uint8_t>(address >> 16);
  buffer[3] = static_cast<uint8_t>(address >> 24);
  buffer[4] = 0;
  return buffer;
}

}  // anonymous namespace

NRFLink::NRFLink(uint32_t address, uint8_t channel, uint16_t ce_pin)
    : Link(address),
      radio_(ce_pin, 0),
      state_(RadioState::UNKNOWN),
      last_transmit_address_(0) {
  // Configure the radio and ensure that it is available.
  CHECK(address != 0, "Address cannot be 0");
  CHECK(address != kBroadcastAddress, "Cannot use the broadcast address");
  CHECK(channel < 128, "Channel must be between 0 and 127");
  CHECK(radio_.begin(), "Failed to start NRF24L01");
  radio_.setChannel(channel);
  radio_.setPALevel(RF24_PA_MAX);
  radio_.setDataRate(RF24_2MBPS);
  radio_.setAddressWidth(4);
  radio_.setAutoAck(0);
  radio_.setCRCLength(RF24_CRC_16);
  CHECK(radio_.isChipConnected(), "NRF24L01 is unavailable");

  // Open reading pipes for the broadcast address and address of this node.
  std::array<uint8_t, 5> address_buffer = FormatAddress(kBroadcastAddress);
  radio_.openReadingPipe(kBroadcastPipe, address_buffer.data());
  address_buffer = FormatAddress(address);
  radio_.openReadingPipe(kDirectedPipe, address_buffer.data());
}

Link::TransmitResult NRFLink::Beacon() {
  RawFrame raw_frame = {};
  PopulateAddress(&raw_frame);

  StartTransmitting(kBroadcastAddress);
  if (!radio_.write(raw_frame.data(), raw_frame.size())) {
    LOGE("Failed to write beacon");
    return TransmitResult::TRANSMIT_ERROR;
  }

  // TODO(aarossig): Handle this potential infinite loop condition.
  while (!radio_.txStandBy()) {
    LOGI("Waiting for beacon transmit standby");
  }

  return TransmitResult::SUCCESS;
}

Link::ReceiveResult NRFLink::Receive(Frame* frame) {
  StartReceiving();
  uint8_t pipe_id = UINT8_MAX;
  if (!radio_.available(&pipe_id)) {
    return ReceiveResult::NOT_READY;
  }

  RawFrame raw_frame = {};
  radio_.read(raw_frame.data(), raw_frame.size());
  if (pipe_id != kBroadcastPipe && pipe_id != kDirectedPipe) {
    LOGW("Received packet from invalid pipe: %" PRIu8, pipe_id);
    return ReceiveResult::RECEIVE_ERROR;
  }

  frame->address = (raw_frame[0])
    | (static_cast<uint32_t>(raw_frame[1]) << 8)
    | (static_cast<uint32_t>(raw_frame[2]) << 16)
    | (static_cast<uint32_t>(raw_frame[3]) << 24);

  if (pipe_id == kDirectedPipe) {
    frame->payload = std::string(raw_frame.begin() + 4, raw_frame.end());
  }

  return ReceiveResult::SUCCESS;
}

Link::TransmitResult NRFLink::Transmit(const Frame& frame) {
  if (frame.payload.size() > GetMaxPayloadSize()) {
    return TransmitResult::TOO_LARGE;
  }

  RawFrame raw_frame = {};
  PopulateAddress(&raw_frame);
  for (size_t i = 0; i < frame.payload.size(); i++) {
    raw_frame[i + 4] = frame.payload[i];
  }

  StartTransmitting(frame.address);
  if (!radio_.write(raw_frame.data(), raw_frame.size())) {
    LOGE("Failed to write beacon");
    return TransmitResult::TRANSMIT_ERROR;
  }

  // TODO(aarossig): Handle this potential infinite loop condition.
  while (!radio_.txStandBy()) {
    LOGI("Waiting for beacon transmit standby");
  }

  return TransmitResult::SUCCESS;
}

uint32_t NRFLink::GetMaxPayloadSize() const {
  return kRawFrameSize - sizeof(uint32_t);
}

void NRFLink::PopulateAddress(RawFrame* raw_frame) {
  raw_frame->at(0) = static_cast<uint8_t>(address());
  raw_frame->at(1) = static_cast<uint8_t>(address() >> 8);
  raw_frame->at(2) = static_cast<uint8_t>(address() >> 16);
  raw_frame->at(3) = static_cast<uint8_t>(address() >> 24);
}

void NRFLink::StartReceiving() {
  if (state_ != RadioState::RECEIVING) {
    radio_.startListening();
    state_ = RadioState::RECEIVING;
  }
}

void NRFLink::StartTransmitting(uint32_t address) {
  bool open_writing_pipe = address != last_transmit_address_;
  if (state_ != RadioState::TRANSMITTING) {
    radio_.stopListening();
    state_ = RadioState::TRANSMITTING;
    open_writing_pipe = true;
  }

  if (open_writing_pipe) {
    std::array<uint8_t, 5> address_buffer = FormatAddress(address);
    radio_.openWritingPipe(address_buffer.data());
  }
}

}  // namespace nerfnet
