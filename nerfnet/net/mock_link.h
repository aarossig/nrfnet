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

#include <condition_variable>
#include <mutex>

namespace nerfnet {

// A mock link implementation used for unit testing.
class MockLink : public Link {
 public:
  struct TestOperation {
    enum class Operation {
      BEACON,
    };

    Operation operation;
    uint64_t expected_time_us;

    union {
      struct {
        TransmitResult result;
      } beacon;
    };
  };

  // Configure the MockLink with the address of this node.
  MockLink(uint32_t address, uint32_t max_payload_size,
      const std::vector<TestOperation>& operations);

  // Waits for the test to finish execution.
  void WaitForComplete();

  // Link implementation.
  TransmitResult Beacon() final;
  ReceiveResult Receive(Frame* frame) final;
  TransmitResult Transmit(const Frame& frame) final;
  uint32_t GetMaxPayloadSize() const final;

 private:
  // The configured maximum payload size.
  const uint32_t max_payload_size_;

  // The list of operations that the mock is expected to execute.
  const std::vector<TestOperation> operations_;

  // The start time of the link for timestamp comparisons.
  const uint64_t start_time_us_;

  // The index of the next operation.
  size_t next_operation_index_;

  // Synchronization.
  std::condition_variable cv_;
  std::mutex mutex_;

  // Returns the time relative to the start of this link.
  uint64_t RelativeTimeUs() const;
};

}  // namespace nerfnet

#endif  // NERFNET_NET_MOCK_LINK_H_
