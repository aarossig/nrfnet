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
      transport_running_(true),
      beacon_thread_(&RadioTransport::BeaconThread, this),
      receive_thread_(&RadioTransport::ReceiveThread, this) {}

RadioTransport::~RadioTransport() {
  transport_running_ = false;
  beacon_thread_.join();
  receive_thread_.join();
}

Transport::SendResult RadioTransport::Send(const NetworkFrame& frame,
    uint32_t address, uint64_t timeout_us) {
  // Serialize the frame.
  std::string serialized_frame;
  if (!frame.SerializeToString(&serialized_frame)) {
    return SendResult::INVALID_FRAME;
  }

  size_t max_payload_size = link()->GetMaxPayloadSize();
  if (max_payload_size <= 2 || max_payload_size > INT_MAX) {
    return SendResult::TOO_LARGE;
  }

  max_payload_size -= 2;
  max_payload_size = std::min(static_cast<int>(max_payload_size), UINT8_MAX)
      * 8 * max_payload_size;
  if (serialized_frame.size() > max_payload_size) {
    return SendResult::TOO_LARGE;
  }

  std::unique_lock<std::mutex> lock(link_mutex_);
  // TODO(aarossig): Send the frame.
  return SendResult::SUCCESS;
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
    {
      Link::Frame frame;
      std::unique_lock<std::mutex> lock(link_mutex_);
      Link::ReceiveResult receive_result = link()->Receive(&frame);
      if (receive_result == Link::ReceiveResult::SUCCESS) {
        if (frame.payload.empty()) {
          event_handler()->OnBeaconReceived(frame.address);
        } else {
          // TODO(aarossig): Finish the receive operation.
        }
      }
    }

    // TODO(aarossig): Rate limit this thread based on receive activity.
    SleepUs(100000);
  }
}

}  // namespace nerfnet
