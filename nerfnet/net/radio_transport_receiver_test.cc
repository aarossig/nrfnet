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

#include "nerfnet/net/radio_transport_receiver.h"
#include "nerfnet/util/log.h"

namespace nerfnet {
namespace {

/* Frame Tests ****************************************************************/

TEST(RadioTransportReceiverFrameTest, BuildBeginFrame) {
  Link::Frame frame = BuildBeginEndFrame(9001, FrameType::BEGIN,
      /*ack=*/false, /*max_payload_size=*/32);
  EXPECT_EQ(frame.address, 9001);
  ASSERT_EQ(frame.payload.size(), 32);
  EXPECT_EQ(frame.payload[0], static_cast<char>(FrameType::BEGIN));
}

TEST(RadioTransportReceiverFrameTest, BuildEndFrame) {
  Link::Frame frame = BuildBeginEndFrame(9002, FrameType::END,
      /*ack=*/true, /*max_payload_size=*/32);
  EXPECT_EQ(frame.address, 9002);
  ASSERT_EQ(frame.payload.size(), 32);
  EXPECT_EQ(frame.payload[0], static_cast<char>(FrameType::END) | (1 << 2));
}

/* Receiver Tests *************************************************************/

// The RadioTransportReceiver test harness.
class RadioTransportReceiverTest : public ::testing::Test,
                                   public Link {
 protected:
  RadioTransportReceiverTest()
      : Link(/*address=*/1000),
        receiver_(&clock_, this) {}

  Link::TransmitResult Beacon() final {
    CHECK(false, "Beacon is not supported");
  }

  Link::ReceiveResult Receive(Link::Frame* frame) final {
    CHECK(false, "Receive is not supported");
  }

  Link::TransmitResult Transmit(const Link::Frame& frame) final {
    last_send_frame_ = frame;
    return next_send_result_;
  }

  uint32_t GetMaxPayloadSize() const final { return 32; }

  MockClock clock_;
  RadioTransportReceiver receiver_;

  // The next send and expected frame.
  Link::TransmitResult next_send_result_;
  Link::Frame last_send_frame_;
};

TEST_F(RadioTransportReceiverTest, PrimeReceiverThenTimeout) {
  EXPECT_FALSE(receiver_.receive_state().has_value());
  clock_.SetTimeUs(1000);

  Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
      /*ack=*/false, /*max_payload_size=*/32);
  receiver_.HandleFrame(frame);

  ASSERT_TRUE(receiver_.receive_state().has_value());
  auto receive_state = receiver_.receive_state().value();
  EXPECT_EQ(receive_state.address, 2000);
  EXPECT_TRUE(receive_state.frame_pieces.empty());
  EXPECT_TRUE(receive_state.frame.empty());
  EXPECT_EQ(receive_state.receive_time_us, 1000);

  clock_.SetTimeUs(1000 + kReceiveTimeoutUs + 1);
  frame.payload = "\x02\x00" + std::string(30, '\x00');
  receiver_.HandleFrame(frame);
  EXPECT_FALSE(receiver_.receive_state().has_value());
}

}  // namespace
}  // namespace nerfnet
