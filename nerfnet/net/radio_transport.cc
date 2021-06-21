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

RadioTransport::RadioTransport(Link* link, EventHandler* event_handler)
    : Transport(link, event_handler),
      transport_thread_running_(true),
      transport_thread_(&RadioTransport::TransportThread, this) {}

RadioTransport::~RadioTransport() {
  transport_thread_running_ = false;
  transport_thread_.join();
}

Transport::SendResult RadioTransport::Send(const NetworkFrame& frame,
    uint32_t address, uint64_t timeout_us) {
  std::string serialized_frame;
  if (!frame.SerializeToString(&serialized_frame)) {
    LOGE("Failed to serialize frame");
    return SendResult::INVALID_FRAME;
  }

  // TODO(aarossig): Fragment and send.

  return SendResult::SUCCESS;
}

void RadioTransport::TransportThread() {
  while (transport_thread_running_) {
    LOGI("TODO: poll");
    SleepUs(200000);
  }
}

}  // namespace nerfnet
