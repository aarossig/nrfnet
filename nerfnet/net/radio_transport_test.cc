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
#include "nerfnet/util/encode_decode.h"

namespace nerfnet {
namespace {

/* Common Transport Test ******************************************************/

const RadioTransport::Config kTestRadioTransportConfig = {
  /*beacon_interval_us=*/100000,  // 100ms.
};

// A super class for test fixtures for the RadioTransport.
class RadioTransportTest : public ::testing::Test,
                           public Transport::EventHandler {
 protected:
  RadioTransportTest(const MockLink::Config& config)
      : link_(config, /*address=*/1000),
        transport_(&link_, this, kTestRadioTransportConfig),
        beacon_failed_count_(0),
        beacon_count_(0) {}

  // Transport::EventHandler implementation.
  void OnBeaconFailed(Link::TransmitResult status) final {
    std::unique_lock<std::mutex> lock(mutex_);
    beacon_failed_count_++;
  }

  void OnBeaconReceived(uint32_t address) final {
    std::unique_lock<std::mutex> lock(mutex_);
    beacon_count_++;
  }

  void OnFrameReceived(uint32_t address, const std::string& frame) final {
    std::unique_lock<std::mutex> lock(mutex_);
    received_frames_.push_back({address, frame});
  }

  // Synchronization.
  std::mutex mutex_;

  // The transport instance to test and underlying mock link.
  MockLink link_;
  RadioTransport transport_;

  // A counter of the number of beacon failures.
  int beacon_failed_count_;

  // A counter of the number of beacons received.
  int beacon_count_;

  // The list of received frames.
  std::vector<std::pair<uint8_t, std::string>> received_frames_;
};

/* Beacon Test ****************************************************************/

class RadioTransportBeaconTest : public RadioTransportTest {
 protected:
  RadioTransportBeaconTest() : RadioTransportTest({
      /*mock_time_us=*/350000,
      /*max_payload_size=*/32,
      /*beacon_interval_us=*/100000,
      /*beacon_result_pattern=*/{
          Link::TransmitResult::SUCCESS,
          Link::TransmitResult::SUCCESS,
          Link::TransmitResult::SUCCESS,
          Link::TransmitResult::TRANSMIT_ERROR,
      },
      /*receive_result=*/{
          {
              Link::ReceiveResult::SUCCESS, {
                  /*address=*/2000, /*payload=*/{},
              },
          },
      },
  }) {}
};

TEST_F(RadioTransportBeaconTest, Beacon) {
  link_.WaitForComplete();
  std::unique_lock<std::mutex> lock(mutex_);
  EXPECT_EQ(beacon_failed_count_, 1);
  EXPECT_EQ(beacon_count_, 1);
}

/* Send Test ******************************************************************/

class RadioTransportSendTest : public RadioTransportTest {
 protected:
  RadioTransportSendTest() : RadioTransportTest({
      /*mock_time_us=*/0,
      /*max_payload_size=*/8,
      /*beacon_interval_us=*/100000,
      /*beacon_result_pattern=*/{
          Link::TransmitResult::SUCCESS,
      },
      /*receive_result=*/{
          {
              Link::ReceiveResult::SUCCESS, {
                  /*address=*/2000, /*payload=*/{
                    0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                  },
              },
          }, {
              Link::ReceiveResult::SUCCESS, {
                  /*address=*/2000, /*payload=*/{
                    0x02, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
                  },
              },
          },
      },
      /*transmit_result=*/{
          {
              Link::TransmitResult::SUCCESS, {
                  /*address=*/2000, /*payload=*/{
                      0x01, 0x00, 0x0c, 0x00, 0x01, 0x02, 0x03, 0x04,
                  },
              },
          }, {
              Link::TransmitResult::SUCCESS, {
                  /*address=*/2000, /*payload=*/{
                      0x00, 0x01, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                  },
              },
          }, {
              Link::TransmitResult::SUCCESS, {
                  /*address=*/2000, /*payload=*/{
                      0x02, 0x02, 0x0b, 0x0c, 0x00, 0x00, 0x00, 0x00,
                  },
              },
          },
      },
  }) {}
};

#if 0
TEST_F(RadioTransportSendTest, Send) {
  EXPECT_EQ(transport_.Send("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c",
      /*address=*/2000, /*timeout_us=*/10000), Transport::SendResult::SUCCESS);
  link_.WaitForComplete();
  std::unique_lock<std::mutex> lock(mutex_);
}
#endif

/* Receive Test ***************************************************************/

class RadioTransportReceiveTest : public RadioTransportTest {
 protected:
  RadioTransportReceiveTest() : RadioTransportTest({
      /*mock_time_us=*/1000,
      /*max_payload_size=*/8,
      /*beacon_interval_us=*/100000,
      /*beacon_result_pattern=*/{
          Link::TransmitResult::SUCCESS,
      },
      /*receive_result=*/{
          {
              Link::ReceiveResult::SUCCESS, {
                  /*address=*/2000, /*payload=*/{
                    0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                  },
              },
          }, {
              Link::ReceiveResult::SUCCESS, {
                  /*address=*/2000, /*payload=*/{
                    0x02, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
                  },
              },
          },
      },
      /*transmit_result=*/{
      },
  }) {}
};

#if 0
TEST_F(RadioTransportReceiveTest, Receive) {
  link_.WaitForComplete();
  std::unique_lock<std::mutex> lock(mutex_);
  ASSERT_EQ(received_frames_.size(), 1);
}
#endif

}  // anonymous namespace
}  // namespace nerfnet
