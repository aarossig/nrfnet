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

#include <gtest/gtest.h>

#include "nerfnet/net/mock_link.h"
#include "nerfnet/net/radio_transport.h"

#include "nerfnet/util/log.h"

namespace nerfnet {
namespace {

RadioTransportConfig GetTestRadioTransportConfig() {
  RadioTransportConfig config;
  config.set_beacon_interval_us(100000);
  return config;
}

// A test fixture for the RadioTransport.
class RadioTransportTest : public ::testing::Test,
                           public Transport::EventHandler {
 protected:
  RadioTransportTest()
      : link_(/*address=*/1000, /*max_payload_size=*/32, {
          MockLink::TestOperation::Beacon(0,
              Link::TransmitResult::SUCCESS),
          MockLink::TestOperation::Beacon(100000,
              Link::TransmitResult::SUCCESS),
          MockLink::TestOperation::Beacon(200000,
              Link::TransmitResult::SUCCESS),
          MockLink::TestOperation::Beacon(300000,
              Link::TransmitResult::TRANSMIT_ERROR),
          MockLink::TestOperation::Delay(350000),
        }),
        transport_(GetTestRadioTransportConfig(), &link_, this),
        beacon_failed_count_(0) {}

  // Transport::EventHandler implementation.
  void OnBeaconFailed(Link::TransmitResult status) final {
    std::unique_lock<std::mutex> lock(mutex_);
    beacon_failed_count_++;
  }
  void OnBeaconReceived(uint32_t address) final {}

  // Synchronization.
  std::mutex mutex_;

  // The transport instance to test and underlying mock link.
  MockLink link_;
  RadioTransport transport_;

  // A counter of the number of beacon failures.
  int beacon_failed_count_;
};

TEST_F(RadioTransportTest, Beacon) {
  link_.WaitForComplete();
  std::unique_lock<std::mutex> lock(mutex_);
  EXPECT_EQ(beacon_failed_count_, 1);
}

}  // anonymous namespace
}  // namespace nerfnet
