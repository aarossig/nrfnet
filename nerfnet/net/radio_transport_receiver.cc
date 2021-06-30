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

#include "nerfnet/net/radio_transport_receiver.h"

#include "nerfnet/util/log.h"

namespace nerfnet {

Link::Frame BuildBeginEndFrame(uint32_t address, FrameType frame_type,
    bool ack, size_t max_payload_size) {
  CHECK(frame_type == FrameType::BEGIN || frame_type == FrameType::END,
      "Frame type must be BEGIN or END");

  Link::Frame frame;
  frame.address = address;
  frame.payload = std::string(max_payload_size, '\0');
  frame.payload[0] = static_cast<uint8_t>(frame_type) | (ack << 2);
  return frame;
}

Link::Frame BuildPayloadFrame(uint32_t address, uint8_t sequence_id,
    const std::string& payload, size_t max_payload_size) {
  const size_t expected_payload_size = max_payload_size - 2;
  CHECK(payload.size() == expected_payload_size,
      "Invalid payload frame size (%zu vs expected %zu)",
      payload.size(), expected_payload_size);

  Link::Frame frame;
  frame.address = address;
  frame.payload = std::string(2, '\0');
  frame.payload[1] = sequence_id;
  frame.payload += payload;
  return frame;
}

// Returns the maximum sub-frame size.
size_t GetMaxSubFrameSize(uint32_t max_payload_size) {
  size_t payload_size = max_payload_size - 2;
  return payload_size * 8 * payload_size;
}

std::vector<std::string> BuildSubFrames(const std::string& frame,
    size_t max_sub_frame_size) {
  // The maximum size of a sub frame is equal to the maximum sub frame minus
  // space for a 4 byte length + 4 byte offset + 4 byte total length.
  const size_t max_sub_frame_payload_length = max_sub_frame_size - 12;

  std::vector<std::string> sub_frames;
  for (size_t sub_frame_offset = 0;
       sub_frame_offset < frame.size();
       sub_frame_offset+= max_sub_frame_payload_length) {
    size_t sub_frame_size = std::min(max_sub_frame_payload_length,
        frame.size() - sub_frame_offset);

    std::string sub_frame;
    sub_frame += EncodeU32(sub_frame_size);
    sub_frame += EncodeU32(sub_frame_offset);
    sub_frame += EncodeU32(frame.size());
    sub_frame += frame.substr(sub_frame_offset, sub_frame_size);
    sub_frames.push_back(sub_frame);
  }

  return sub_frames;
}

RadioTransportReceiver::RadioTransportReceiver(const Clock* clock, Link* link)
    : clock_(clock), link_(link) {}

std::optional<std::string> RadioTransportReceiver::HandleFrame(
    const Link::Frame& frame) {
  HandleTimeout();

  FrameType frame_type = static_cast<FrameType>(
      frame.payload[0] & kMaskFrameType);
  bool frame_ack = (frame.payload[0] & kMaskAck) > 0;
  if (!receive_state_.has_value()) {
    if (frame_type == FrameType::BEGIN && !frame_ack) {
      // Initialize the receiver state.
      receive_state_.emplace();
      receive_state_->address = frame.address;
      receive_state_->receive_time_us = clock_->TimeNowUs();
      RespondWithAck(FrameType::BEGIN);
    }
  } else if (receive_state_.has_value()) {
    if (frame_type == FrameType::BEGIN && !frame_ack) {
      RespondWithAck(FrameType::BEGIN);
    } else if (frame_type == FrameType::PAYLOAD) {
      uint8_t sequence_id = frame.payload[1];
      if (receive_state_->pieces.find(sequence_id)
          != receive_state_->pieces.end()) {
        receive_state_->pieces[sequence_id] = frame.payload.substr(2);
      }
    } else if (frame_type == FrameType::END && !frame_ack) {
      RespondWithAck(FrameType::END);
      if (receive_state_.has_value()) {
        return HandleCompleteReceiveState();
      }
    }
  }

  return std::nullopt;
}

void RadioTransportReceiver::HandleTimeout() {
  if (receive_state_.has_value()) {
    if ((clock_->TimeNowUs() - receive_state_->receive_time_us)
            > kReceiveTimeoutUs) {
      receive_state_.reset();
    }
  }
}

void RadioTransportReceiver::RespondWithAck(FrameType frame_type) {
  Link::Frame ack_frame = BuildBeginEndFrame(receive_state_->address,
      frame_type, /*ack=*/true, link_->GetMaxPayloadSize());
  for (const auto& piece : receive_state_->pieces) {
    size_t byte_index = (piece.first / 8) + 2;
    size_t bit_index = piece.first % 8;
    ack_frame.payload[byte_index] |= (1 << bit_index);
  }

  Link::TransmitResult transmit_result = link_->Transmit(ack_frame);
  if (transmit_result != Link::TransmitResult::SUCCESS) {
    LOGE("Failed to transmit BEGIN ack: %d", transmit_result);
    receive_state_.reset();
  }
}

std::optional<std::string> RadioTransportReceiver::HandleCompleteReceiveState() {
  // TODO(aarossig): Keep the last details around.
  // TODO(aarossig): Decode frame, append to frame.
  return std::nullopt;
}

}  // namespace nerfnet
