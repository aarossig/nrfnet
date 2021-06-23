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

RadioTransport::RadioTransport(const RadioTransportConfig& config,
    Link* link, EventHandler* event_handler)
    : Transport(link, event_handler),
      config_(config),
      transport_thread_running_(true),
      transport_thread_(&RadioTransport::TransportThread, this),
      send_frame_(nullptr) {}

RadioTransport::~RadioTransport() {
  transport_thread_running_ = false;
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

  // Signal the transport thread that there is a frame to send and wait up to
  // the specified timeout for that to complete.
  std::unique_lock<std::mutex> lock(mutex_);
  send_frame_ = &serialized_frame;
  bool result = cv_.wait_for(lock, std::chrono::microseconds(timeout_us),
      [this]() { return send_frame_ == nullptr; });
  send_frame_ = nullptr;
  return result ? SendResult::SUCCESS : SendResult::TIMEOUT;
}

void RadioTransport::TransportThread() {
  while (transport_thread_running_) {
    LOGI("TODO: poll");
    SleepUs(200000);
  }
}

}  // namespace nerfnet
