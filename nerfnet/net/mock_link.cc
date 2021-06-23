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

#include "nerfnet/net/mock_link.h"

namespace nerfnet {

MockLink::MockLink(uint32_t address, uint32_t max_payload_size)
    : Link(address),
      max_payload_size_(max_payload_size) {}

Link::TransmitResult MockLink::Beacon() {
  // TODO(aarossig): Provide a mocking mechanism.
  return TransmitResult::SUCCESS;
}

Link::ReceiveResult MockLink::Receive(Frame* frame) {
  // TODO(aarossig): Provide a mocking mechanism.
  return ReceiveResult::SUCCESS;
}

Link::TransmitResult MockLink::Transmit(const Frame& frame) {
  // TODO(aarossig): Provide a mocking mechanism.
  return TransmitResult::SUCCESS;
}

uint32_t MockLink::GetMaxPayloadSize() const {
  return max_payload_size_;
}


}  // namespace nerfnet
