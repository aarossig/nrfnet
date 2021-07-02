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

#include <set>

#include "nerfnet/util/encode_decode.h"
#include "nerfnet/util/log.h"
#include "nerfnet/util/rand.h"
#include "nerfnet/util/time.h"

namespace nerfnet {

const RadioTransport::Config RadioTransport::kDefaultConfig = {
  /*beacon_interval_us=*/100000,  // 100ms.
};

RadioTransport::RadioTransport(Link* link, EventHandler* event_handler,
    const Config& config)
    : Transport(link, event_handler),
      config_(config),
      last_beacon_time_us_(0),
      receiver_(&clock_, link),
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
  const std::vector<std::string> sub_frames = BuildSubFrames(frame,
      GetMaxSubFrameSize(link()->GetMaxPayloadSize()));

  std::unique_lock<std::mutex> lock(link_mutex_);
  for (const auto& sub_frame : sub_frames) {
    // Send BEGIN frame.
    SendResult send_result = SendReceiveBeginEndFrame(FrameType::BEGIN, address,
        start_time_us, timeout_us);
    if (send_result != SendResult::SUCCESS) {
      return send_result;
    }

    // Transmit all frames that have not been acknowledged.
    const size_t payload_chunk_size = link()->GetMaxPayloadSize() - 2;
    const uint8_t max_sequence_id = (sub_frame.size() / payload_chunk_size) + 1;
    std::set<uint8_t> acknowledged_ids;
    while (acknowledged_ids.size() < max_sequence_id) {
      uint8_t sequence_id = 0;
      for (size_t offset = 0; offset < sub_frame.size();
           offset += payload_chunk_size) {
        if (acknowledged_ids.find(sequence_id) == acknowledged_ids.end()) {
          // The sequence ID has not been transmitted yet, so send it.
          Link::Frame frame = BuildPayloadFrame(address, sequence_id,
              sub_frame.substr(offset, payload_chunk_size),
              link()->GetMaxPayloadSize());

          // Transmit the frame and log errors as warnings. The receiving radio
          // will fail to acknowledge any missing sequence IDs and this radio
          // will retry transmission.
          Link::TransmitResult transmit_result = link()->Transmit(frame);
          if (transmit_result != Link::TransmitResult::SUCCESS) {
            LOGW("Failed to transmit sequence_id=%u with %u",
                sequence_id, transmit_result);
          }
        }

        sequence_id++;
      }

      // Transmit END frame.
      Link::Frame ack_frame;
      send_result = SendReceiveBeginEndFrame(FrameType::END, address,
          start_time_us, timeout_us, &ack_frame);
      if (send_result != SendResult::SUCCESS) {
        LOGE("Failed to send/receive end frame: %d", send_result);
        return send_result;
      }

      // Parse received acks and insert into the set of acknowledged IDs.
      for (sequence_id = 0; sequence_id < max_sequence_id; sequence_id++) {
        uint8_t byte_index = (sequence_id / 8) + 2;
        uint8_t bit_index = sequence_id % 8;
        if ((ack_frame.payload[byte_index] & (1 << bit_index)) > 0) {
          acknowledged_ids.insert(sequence_id);
        }
      }
    }
  }

  return SendResult::SUCCESS;
}

void RadioTransport::BeaconThread() {
  while (transport_running_) {
    uint64_t time_now_us = TimeNowUs();
    int64_t beacon_interval_error_us = config_.beacon_interval_us / 10;
    int64_t beacon_jitter_us = Random<int64_t>(
        -beacon_interval_error_us, beacon_interval_error_us);
    if ((time_now_us - last_beacon_time_us_)
        > (config_.beacon_interval_us + beacon_jitter_us)) {
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
    bool frame_received = false;

    {
      Link::Frame frame;
      std::unique_lock<std::mutex> lock(link_mutex_);
      Link::ReceiveResult receive_result = link()->Receive(&frame);
      if (receive_result == Link::ReceiveResult::SUCCESS) {
        frame_received = true;
        if (frame.payload.empty()) {
          event_handler()->OnBeaconReceived(frame.address);
        } else if (frame.payload.size() != link()->GetMaxPayloadSize()) {
          LOGW("Received frame length mismatch (%zu vs expected %zu",
              frame.payload.size(), link()->GetMaxPayloadSize());
        } else {
          HandlePayloadFrame(frame);
        }
      } else if (receive_result != Link::ReceiveResult::NOT_READY) {
        LOGW("Failed to receive frame: %u", receive_result);
      }
    }

    if (!frame_received) {
      SleepUs(1000);
    }
  }
}

Transport::SendResult RadioTransport::SendReceiveBeginEndFrame(
    FrameType frame_type, uint32_t address,
    uint64_t start_time_us, uint64_t timeout_us,
    Link::Frame* out_frame) {
  Link::ReceiveResult receive_result = Link::ReceiveResult::NOT_READY;
  while (receive_result != Link::ReceiveResult::SUCCESS) {
    if ((TimeNowUs() - start_time_us) > timeout_us) {
      return SendResult::TIMEOUT;
    }

    Link::Frame frame = BuildBeginEndFrame(address, frame_type, /*ack=*/false,
        link()->GetMaxPayloadSize());
    Link::TransmitResult transmit_result = link()->Transmit(frame);
    if (transmit_result != Link::TransmitResult::SUCCESS) {
      LOGE("Failed to transmit frame: %d", transmit_result);
      continue;
    }

    uint64_t receive_start_time_us = TimeNowUs();
    while (receive_result != Link::ReceiveResult::SUCCESS) {
      if ((TimeNowUs() - receive_start_time_us) > kReceiveTimeoutUs) {
        break;
      }

      receive_result = link()->Receive(&frame);
      if (receive_result == Link::ReceiveResult::SUCCESS) {
        if (frame.payload.empty()) {
          event_handler()->OnBeaconReceived(frame.address);
        } else if (frame.address != address) {
          LOGW("Ignoring frame from %u while in transmission to %u",
              frame.address, address);
        } else if (frame.payload.size() != link()->GetMaxPayloadSize()) {
          LOGW("Received frame from %u with frame size %zu vs expected %zu",
              frame.address, frame.payload.size(),
              link()->GetMaxPayloadSize());
        } else if ((frame.payload[0] & kMaskFrameType)
            != static_cast<uint8_t>(frame_type)) {
          LOGW("Received frame from %u with unexpected frame type %d "
              "vs expected %d",
              frame.address, frame.payload[0] & kMaskFrameType, frame_type);
        } else if (frame.payload[0] & kMaskAck == 0) {
          LOGW("Received frame from %u missing expected ack", frame.address);
        } else {
          break;
        }

        receive_result = Link::ReceiveResult::NOT_READY;
      } else if (receive_result != Link::ReceiveResult::NOT_READY) {
        LOGE("Failed to receive: %d", receive_result);
      }
    }

    if (receive_result == Link::ReceiveResult::SUCCESS
        && out_frame != nullptr) {
      *out_frame = frame;
    }
  }

  return SendResult::SUCCESS;
}

void RadioTransport::HandlePayloadFrame(const Link::Frame& frame) {
  std::optional<std::string> payload = receiver_.HandleFrame(frame);
  if (payload.has_value()) {
    event_handler()->OnFrameReceived(frame.address, *payload);
  }
}

}  // namespace nerfnet
