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

#ifndef NERFNET_NET_MOCK_LINK_H_
#define NERFNET_NET_MOCK_LINK_H_

#include "nerfnet/net/link.h"

namespace nerfnet {

// A mock link implementation used for unit testing.
class MockLink : public Link {
 public:
  // Configure the MockLink with the address of this node.
  MockLink(uint32_t address, uint32_t max_payload_size);

  // Link implementation.
  TransmitResult Beacon() final;
  ReceiveResult Receive(Frame* frame) final;
  TransmitResult Transmit(const Frame& frame) final;
  uint32_t GetMaxPayloadSize() const final;

 private:
  // The configured maximum payload size.
  const uint32_t max_payload_size_;
};

}  // namespace nerfnet

#endif  // NERFNET_NET_MOCK_LINK_H_
