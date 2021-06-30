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
      // TODO(aarossig): Clear the receive state, but keep the last details around.
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
  Link::TransmitResult transmit_result = link_->Transmit(ack_frame);
  if (transmit_result != Link::TransmitResult::SUCCESS) {
    LOGE("Failed to transmit BEGIN ack: %d", transmit_result);
    receive_state_.reset();
  }
}

}  // namespace nerfnet
