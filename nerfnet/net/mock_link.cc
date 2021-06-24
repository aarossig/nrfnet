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

#include <gtest/gtest.h>

#include "nerfnet/util/log.h"
#include "nerfnet/util/time.h"

namespace nerfnet {

MockLink::MockLink(uint32_t address, uint32_t max_payload_size,
    const std::vector<TestOperation>& operations)
    : Link(address),
      max_payload_size_(max_payload_size),
      operations_(operations),
      start_time_us_(TimeNowUs()),
      next_operation_index_(0) {
  CHECK(!operations_.empty(), "MockLink operations must be non-empty");
}

void MockLink::WaitForComplete() {
  while (true) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (next_operation_index_ >= operations_.size()) {
      break;
    }

    cv_.wait(lock);
  }
}

Link::TransmitResult MockLink::Beacon() {
  uint64_t relative_time_us = RelativeTimeUs();

  std::unique_lock<std::mutex> lock(mutex_);
  CHECK(next_operation_index_ < operations_.size(),
      "next_operation_index_ out of bounds");
  const TestOperation& operation = operations_[next_operation_index_++];
  EXPECT_GE(relative_time_us, operation.expected_time_us);
  EXPECT_LT(relative_time_us, operation.expected_time_us + 1000);
  cv_.notify_one();
  return operation.beacon.result;
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

uint64_t MockLink::RelativeTimeUs() const {
  return TimeNowUs() - start_time_us_;
}

}  // namespace nerfnet
