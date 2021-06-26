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

#include "nerfnet/util/log.h"
#include "nerfnet/util/time.h"

namespace nerfnet {

const RadioTransport::Config RadioTransport::kDefaultConfig = {
  /*beacon_interval_us=*/100000,  // 100ms.
};

RadioTransport::RadioTransport(Link* link, EventHandler* event_handler,
    const Config& config)
    : Transport(link, event_handler),
      config_(config),
      mode_(Mode::IDLE),
      last_beacon_time_us_(0),
      send_frame_(nullptr),
      transport_running_(true),
      beacon_thread_(&RadioTransport::BeaconThread, this),
      transport_thread_(&RadioTransport::TransportThread, this) {}

RadioTransport::~RadioTransport() {
  transport_running_ = false;
  beacon_thread_.join();
  transport_thread_.join();
}

Transport::SendResult RadioTransport::Send(const NetworkFrame& frame,
    uint32_t address, uint64_t timeout_us) {
  // Serialize the frame.
  std::string serialized_frame;
  if (!frame.SerializeToString(&serialized_frame)) {
    LOGE("Failed to serialize frame");
    return SendResult::INVALID_FRAME;
  }

  // TODO(aarossig): Check too large.

  // Signal the transport thread that there is a frame to send and wait up to
  // the specified timeout for that to complete.
  std::unique_lock<std::mutex> lock(mutex_);
  send_frame_ = &serialized_frame;
  send_address_ = address;
  bool result = cv_.wait_for(lock, std::chrono::microseconds(timeout_us),
      [this]() { return send_frame_ == nullptr; });
  send_frame_ = nullptr;
  return result ? SendResult::SUCCESS : SendResult::TIMEOUT;
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

void RadioTransport::TransportThread() {
  while (transport_running_) {
    SleepUs(10000);
  }
}

uint64_t RadioTransport::Send(uint64_t time_now_us) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (send_frame_ == nullptr) {
    return UINT64_MAX;
  }

  if (mode_ == Mode::IDLE) {
    send_frame_offset_ = 0;
    mode_ = Mode::SENDING;
  }

  if (mode_ == Mode::SENDING) {
    // TODO(aarossig): Build the packet, send it.
  }

  return 0;
}

uint64_t RadioTransport::Receive(uint64_t time_now_us) {
  return 0;
}

}  // namespace nerfnet
