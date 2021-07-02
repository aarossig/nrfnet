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

#include <mutex>
#include <vector>

namespace nerfnet {

// A mock link implementation used for unit testing.
class MockLink : public Link {
 public:
  // The configuration for the mock link.
  struct Config {
    // The amount of time for the MockLink to operate.
    uint64_t mock_time_us;

    // The maximum payload size.
    size_t max_payload_size;

    // The expected interval between beacons.
    uint64_t beacon_interval_us;

    // The pattern of beacon results to produce. This will be repated for the
    // duration of the mock link period.
    std::vector<Link::TransmitResult> beacon_result_pattern;

    // The frames for Receive() to provide.
    std::vector<std::pair<ReceiveResult, Frame>> receive_result;

    // The frames for Transmit() to be expected to send.
    std::vector<std::pair<TransmitResult, Frame>> transmit_result;
  };

  // Configure the MockLink with the address of this node.
  MockLink(const Config& config, uint32_t address);

  // Waits for the test to finish execution.
  void WaitForComplete();

  // Link implementation.
  TransmitResult Beacon() final;
  ReceiveResult Receive(Frame* frame) final;
  TransmitResult Transmit(const Frame& frame) final;
  uint32_t GetMaxPayloadSize() const final;

 private:
  // The config to use for this mock link.
  const Config config_;

  // The start time of the link for timestamp comparisons.
  const uint64_t start_time_us_;

  // The number of Beacon() calls that have been performed.
  size_t beacon_count_;

  // The number of Receive() calls that have been performed.
  size_t receive_count_;

  // The number of Transmit() calls that have been performed.
  size_t transmit_count_;

  // Returns the time relative to the start of this link.
  uint64_t RelativeTimeUs() const;
};

}  // namespace nerfnet

#endif  // NERFNET_NET_MOCK_LINK_H_
