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

#include "nerfnet/net/radio_transport.h"

#include <map>
#include <set>

#include "nerfnet/util/log.h"
#include "nerfnet/util/time.h"

namespace nerfnet {
namespace {

// Start/finish packet flags and mask.
constexpr uint8_t kFlagStart = 0x01;
constexpr uint8_t kFlagFinish = 0x02;
constexpr uint8_t kMaskStartFinish = 0x03;

// The maximum amount of time to await a reply when sending/receiving a frame.
constexpr uint64_t kReceiveTimeoutUs = 10000;

}  // anonymous namespace

const RadioTransport::Config RadioTransport::kDefaultConfig = {
  /*beacon_interval_us=*/100000,  // 100ms.
};

RadioTransport::RadioTransport(Link* link, EventHandler* event_handler,
    const Config& config)
    : Transport(link, event_handler),
      config_(config),
      last_beacon_time_us_(0),
      transport_running_(true),
      beacon_thread_(&RadioTransport::BeaconThread, this),
      receive_thread_(&RadioTransport::ReceiveThread, this) {
  // The minimum payload size is 2 bytes of header plus 1 byte of content making
  // the minimum 3 bytes. The sequnce ID is encoded as a single byte which makes
  // the maximum sequence ID 255, which caps the frame size at 257. Enforcing
  // these limits up front simplifies the implementation of the transport.
  constexpr size_t kMinimumPayloadSize = 3;
  constexpr size_t kMaximumPayloadSize = static_cast<size_t>(UINT8_MAX) + 2;
  size_t max_payload_size = link->GetMaxPayloadSize();
  CHECK(max_payload_size >= kMinimumPayloadSize,
      "Link minimum payload size too small (%zu vs expected %zu)",
      max_payload_size, kMinimumPayloadSize);
  CHECK(max_payload_size < kMaximumPayloadSize,
      "Link maximum payload too large (%zu vs max %u)",
      max_payload_size, kMaximumPayloadSize);
}

RadioTransport::~RadioTransport() {
  transport_running_ = false;
  beacon_thread_.join();
  receive_thread_.join();
}

Transport::SendResult RadioTransport::Send(const std::string& frame,
    uint32_t address, uint64_t timeout_us) {
  const uint64_t start_time_us = TimeNowUs();

  size_t max_payload_size = link()->GetMaxPayloadSize();
  if (max_payload_size <= 2 || max_payload_size > INT_MAX) {
    return SendResult::TOO_LARGE;
  }

  // Prepend the length of the frame.
  std::string wire_frame;
  wire_frame.push_back(static_cast<uint8_t>(frame.size()));
  wire_frame.push_back(static_cast<uint8_t>(frame.size() >> 8));
  wire_frame += frame;

  size_t max_fragment_size = max_payload_size - 2;
  size_t max_frame_size = std::min(
      static_cast<int>(max_fragment_size), UINT8_MAX) * 8 * (max_fragment_size);
  if (wire_frame.size() > max_frame_size) {
    return SendResult::TOO_LARGE;
  }

  Link::Frame transmit_frame = {};
  transmit_frame.address = address;

  std::unique_lock<std::mutex> lock(link_mutex_);
  std::set<uint8_t> acknowledged_seq_ids;
  uint8_t max_seq_id = (wire_frame.size() / max_fragment_size) + 1;
  while (acknowledged_seq_ids.size() != max_seq_id) {
    bool receive_timeout = false;
    for (uint8_t seq_id = 0; seq_id < max_seq_id && !receive_timeout; seq_id++) {
      if (acknowledged_seq_ids.find(seq_id) != acknowledged_seq_ids.end()) {
        continue;
      }

      // Build the frame to send over the link.
      const size_t payload_offset = seq_id * max_fragment_size;
      uint8_t flags = seq_id == 0 ? kFlagStart : 0;
      flags |= seq_id == max_seq_id - 1 ? kFlagFinish : 0;
      transmit_frame.payload = BuildPayload(flags, seq_id,
          wire_frame.substr(payload_offset, max_fragment_size));
      transmit_frame.payload.resize(max_payload_size, '\0');

      // Send the frame.
      Link::TransmitResult transmit_result = link()->Transmit(transmit_frame);
      if (transmit_result != Link::TransmitResult::SUCCESS) {
        LOGE("Failed to transmit frame: %d", transmit_result);
        return SendResult::TRANSMIT_ERROR;
      }
   
      // Listen for acks if needed.
      if ((flags & kMaskStartFinish) > 0) {
        Link::Frame receive_frame = {};
        Link::ReceiveResult receive_result;

        uint64_t receive_time_us = TimeNowUs();
        do {
          receive_result = link()->Receive(&receive_frame);
          if ((TimeNowUs() - start_time_us) > timeout_us) {
            return SendResult::TIMEOUT;
          } else if ((TimeNowUs() - receive_time_us) > kReceiveTimeoutUs) {
            receive_timeout = true;
            break;
          }
        } while (receive_result == Link::ReceiveResult::NOT_READY);
    
        if (!receive_timeout) {
          if (receive_result != Link::ReceiveResult::SUCCESS) {
            LOGE("Failed to receive frame: %d", receive_result);
            return SendResult::RECEIVE_ERROR;
          } else if (receive_frame.payload.empty()) {
            event_handler()->OnBeaconReceived(receive_frame.address);
          } else if (receive_frame.payload.size() != max_payload_size) {
            LOGW("Frame length mismatch (%zu vs expected %zu)",
                receive_frame.payload.size(), max_payload_size);
          } else if ((receive_frame.payload[0] & kMaskStartFinish) != flags) {
            LOGW("Invalid flags received (0x%02x vs expected 0x%02x)",
                receive_frame.payload[0] & kMaskStartFinish, flags);
          } else {
            uint8_t check_seq_id = 0;
            for (size_t i = 2;
                 i < receive_frame.payload.size() && check_seq_id < max_seq_id;
                 i++) {
              for (size_t bit_index = 0;
                   bit_index < 8 && check_seq_id < max_seq_id; bit_index++) {
                if (receive_frame.payload[i] & (1 << bit_index)) {
                  acknowledged_seq_ids.insert(check_seq_id);
                }

                check_seq_id++;
              }
            }
          }
        }
      }
    }
  }

  return SendResult::SUCCESS;
}

size_t RadioTransport::GetMaxSubFrameSize() const {
  size_t payload_size = link()->GetMaxPayloadSize() - 2;
  return payload_size * 8 * payload_size;
}

Link::Frame RadioTransport::BuildBeginEndFrame(uint32_t address,
    FrameType frame_type, bool ack) const {
  CHECK(frame_type == FrameType::BEGIN || frame_type == FrameType::END,
      "Frame type must be BEGIN or END");

  Link::Frame frame;
  frame.address = address;
  frame.payload = std::string(link()->GetMaxPayloadSize(), '\0');
  frame.payload[0] = static_cast<uint8_t>(frame_type) | (ack << 2);
  return frame;
}

Link::Frame RadioTransport::BuildPayloadFrame(uint32_t address,
    uint8_t sequence_id, const std::string& payload) const {
  const size_t expected_payload_size = link()->GetMaxPayloadSize() - 2;
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

void RadioTransport::BeaconThread() {
  while(transport_running_) {
    uint64_t time_now_us = TimeNowUs();
    if ((time_now_us - last_beacon_time_us_) > config_.beacon_interval_us) {
      std::unique_lock<std::mutex> lock(link_mutex_);
      Link::TransmitResult result = link()->Beacon();
      if (result != Link::TransmitResult::SUCCESS) {
        event_handler()->OnBeaconFailed(result);
      }
  
      last_beacon_time_us_ = time_now_us;
    }
  
    uint64_t next_beacon_time_us = last_beacon_time_us_
        + config_.beacon_interval_us;
    if (next_beacon_time_us > time_now_us) {
      SleepUs(next_beacon_time_us - time_now_us);
    }
  }
}

void RadioTransport::ReceiveThread() {
  while (transport_running_) {
    // TODO(aarossig): Rate limit this thread based on receive activity.
    SleepUs(10000);

    Link::Frame frame;
    std::unique_lock<std::mutex> lock(link_mutex_);
    Link::ReceiveResult receive_result = link()->Receive(&frame);
    if (receive_result != Link::ReceiveResult::NOT_READY) {
      LOGW("Failed to receive frame: %u", receive_result);
    } else if (receive_result == Link::ReceiveResult::SUCCESS) {
      if (frame.payload.empty()) {
        event_handler()->OnBeaconReceived(frame.address);
      } else if (frame.payload.size() != link()->GetMaxPayloadSize()) {
        LOGW("Received frame length mismatch (%zu vs expected %zu",
            frame.payload.size(), link()->GetMaxPayloadSize());
      } else if ((frame.payload[0] & kFlagStart) != kFlagStart) {
        LOGW("Received non-start frame from %u", frame.address);
      } else {
        bool frame_dispatched = false;
        uint32_t address = frame.address;
        std::map<uint8_t, std::string> received_frames;
        Link::ReceiveResult receive_result = Link::ReceiveResult::SUCCESS;

        while (transport_running_ && !frame_dispatched
               && receive_result == Link::ReceiveResult::SUCCESS) {
          if (frame.payload.empty()) {
            event_handler()->OnBeaconReceived(frame.address);
          } else if (frame.address != address) {
            LOGW("Ignoring packet from %u", frame.address);
          } else {
            uint8_t flags = frame.payload[0];
            // TODO(aarossig): Validate this seq_id.
            uint8_t seq_id = frame.payload[1];
            std::string payload = frame.payload.substr(2);

            if ((flags & kFlagFinish) == kFlagFinish) {
              bool packet_received = true;
              for (uint16_t i = 0; i <= seq_id; i++) {
                if (received_frames.find(i) == received_frames.end()) {
                  packet_received = false;
                  break;
                }
              }

              if (packet_received) {
                size_t length = received_frames[0][0]
                    | (static_cast<uint16_t>(received_frames[0][1]) << 8);
                std::string payload = received_frames[0].substr(2);
                for (uint16_t i = 1; i <= seq_id; i++) {
                  payload += received_frames[i];
                }

                // TODO(aarossig): Validate this length input.
                payload = payload.substr(2, length);
                event_handler()->OnFrameReceived(address, payload);
                frame_dispatched = true;
              }
            }

            if ((flags & kMaskStartFinish) > 0) {
              std::string ack_mask(link()->GetMaxPayloadSize() - 2, '\0');
              for (const auto& received_frame : received_frames) {
                uint8_t seq_id = received_frame.first;
                uint8_t byte = seq_id / 8;
                uint8_t bit = seq_id % 8;
                ack_mask[byte] |= (1 << bit);
              }

              Link::Frame response;
              response.address = address;
              response.payload = BuildPayload(
                  flags & kMaskStartFinish, 0, ack_mask);
              link()->Transmit(response);
            }

          }

          // Receive the next frame.
          uint64_t start_receive_time_us = TimeNowUs();
          Link::ReceiveResult receive_result = Link::ReceiveResult::NOT_READY;
          do {
            if ((TimeNowUs() - start_receive_time_us) > kReceiveTimeoutUs) {
              receive_result = Link::ReceiveResult::NOT_READY;
              break;
            }

            receive_result = link()->Receive(&frame);
          } while (receive_result == Link::ReceiveResult::NOT_READY);
        }
      }
    }
  }
}

std::string RadioTransport::BuildPayload(uint8_t flags, uint8_t sequence_id,
    const std::string& contents) {
  std::string payload;
  payload.push_back(flags);
  payload.push_back(sequence_id);
  payload += contents;
  return payload;
}

}  // namespace nerfnet
