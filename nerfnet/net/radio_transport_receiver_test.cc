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

TEST(RadioTransportReceiverFrameTest, BuildPayloadFrame) {
  Link::Frame frame = BuildPayloadFrame(9003, /*sequence_id=*/10,
      std::string(30, '\xaa'), /*max_payload_size=*/32);
  EXPECT_EQ(frame.address, 9003);
  ASSERT_EQ(frame.payload.size(), 32);
  EXPECT_EQ(frame.payload[0], 0x00);
  EXPECT_EQ(frame.payload[1], 10);
  EXPECT_EQ(frame.payload.substr(2), std::string(30, '\xaa'));
}

TEST(RadioTransportReceiverFrameTest, GetMaxSubFrameSize) {
  EXPECT_EQ(GetMaxSubFrameSize(/*max_payload_size=*/32), 7200);
}

TEST(RadioTransportReceiverFrameTest, BuildSubFramesSingle) {
  std::vector<std::string> sub_frames = BuildSubFrames(
      std::string(16, '\xaa'), GetMaxSubFrameSize(/*max_payload_size=*/32));
  EXPECT_EQ(sub_frames.size(), 1);
  EXPECT_EQ(DecodeU32(
        static_cast<std::string_view>(sub_frames[0]).substr(0)), 16);
  EXPECT_EQ(DecodeU32(
        static_cast<std::string_view>(sub_frames[0]).substr(4)), 0);
  EXPECT_EQ(DecodeU32(
        static_cast<std::string_view>(sub_frames[0]).substr(8)), 16);
  EXPECT_EQ(sub_frames[0].substr(12), std::string(16, '\xaa'));
}

TEST(RadioTransportReceiverFrameTest, BuildSubFramesMulti) {
  std::vector<std::string> sub_frames = BuildSubFrames(
      std::string(8192, '\xbb'), GetMaxSubFrameSize(/*max_payload_size=*/32));
  EXPECT_EQ(sub_frames.size(), 2);

  // Check the first frame.
  ASSERT_EQ(sub_frames[0].size(), 7200);
  EXPECT_EQ(DecodeU32(
        static_cast<std::string_view>(sub_frames[0]).substr(0)), 7188);
  EXPECT_EQ(DecodeU32(
        static_cast<std::string_view>(sub_frames[0]).substr(4)), 0);
  EXPECT_EQ(DecodeU32(
        static_cast<std::string_view>(sub_frames[0]).substr(8)), 8192);
  EXPECT_EQ(sub_frames[0].substr(12), std::string(7188, '\xbb'));

  // Check the second frame.
  ASSERT_EQ(sub_frames[1].size(), 1016);
  EXPECT_EQ(DecodeU32(
        static_cast<std::string_view>(sub_frames[1]).substr(0)), 1004);
  EXPECT_EQ(DecodeU32(
        static_cast<std::string_view>(sub_frames[1]).substr(4)), 7188);
  EXPECT_EQ(DecodeU32(
        static_cast<std::string_view>(sub_frames[1]).substr(8)), 8192);
  EXPECT_EQ(sub_frames[1].substr(12), std::string(1004, '\xbb'));
}

/* Receiver Tests *************************************************************/

// The RadioTransportReceiver test harness.
class RadioTransportReceiverTest : public ::testing::Test,
                                   public Link {
 protected:
  RadioTransportReceiverTest()
      : Link(/*address=*/1000),
        receiver_(&clock_, this),
        transmit_count_(0),
        next_send_result_(Link::TransmitResult::SUCCESS),
        max_payload_size_(32) {}

  Link::TransmitResult Beacon() final {
    CHECK(false, "Beacon is not supported");
  }

  Link::ReceiveResult Receive(Link::Frame* frame) final {
    CHECK(false, "Receive is not supported");
  }

  Link::TransmitResult Transmit(const Link::Frame& frame) final {
    transmit_count_++;
    last_send_frame_ = frame;
    return next_send_result_;
  }

  uint32_t GetMaxPayloadSize() const final { return max_payload_size_; }

  MockClock clock_;
  RadioTransportReceiver receiver_;

  // The next send and expected frame.
  size_t transmit_count_;
  Link::TransmitResult next_send_result_;
  Link::Frame last_send_frame_;

  // The maximum payload size.
  uint32_t max_payload_size_;
};

TEST_F(RadioTransportReceiverTest, PrimeReceiverThenTimeout) {
  EXPECT_FALSE(receiver_.receive_state().has_value());
  clock_.SetTimeUs(1000);

  Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
      /*ack=*/false, /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  ASSERT_TRUE(receiver_.receive_state().has_value());
  auto receive_state = receiver_.receive_state().value();
  EXPECT_EQ(receive_state.address, 2000);
  EXPECT_TRUE(receive_state.pieces.empty());
  EXPECT_TRUE(receive_state.payload.empty());
  EXPECT_EQ(receive_state.receive_time_us, 1000);

  frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
      /*ack=*/true, /*max_payload_size=*/32);
  EXPECT_EQ(transmit_count_, 1);
  EXPECT_EQ(last_send_frame_.address, frame.address);
  EXPECT_EQ(last_send_frame_.payload, frame.payload);

  clock_.SetTimeUs(1000 + kReceiverTimeoutUs + 1);
  frame = BuildBeginEndFrame(2000, FrameType::END,
      /*ack=*/false, /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
  EXPECT_FALSE(receiver_.receive_state().has_value());
  EXPECT_EQ(transmit_count_, 1);
}

TEST_F(RadioTransportReceiverTest, PrimeReceiverAckTransmitFailure) {
  next_send_result_ = Link::TransmitResult::TRANSMIT_ERROR;
  Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
      /*ack=*/false, /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
  EXPECT_FALSE(receiver_.receive_state().has_value());
  EXPECT_EQ(transmit_count_, 1);
}

TEST_F(RadioTransportReceiverTest, PrimeReceiverIgnoreAck) {
  Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
      /*ack=*/true, /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
  EXPECT_FALSE(receiver_.receive_state().has_value());
  EXPECT_EQ(transmit_count_, 0);
}

TEST_F(RadioTransportReceiverTest, PrimeReceiverRepeatAck) {
  Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
      /*ack=*/false, /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
  EXPECT_TRUE(receiver_.receive_state().has_value());
  EXPECT_EQ(transmit_count_, 1);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
  EXPECT_TRUE(receiver_.receive_state().has_value());
  EXPECT_EQ(transmit_count_, 2);
}

TEST_F(RadioTransportReceiverTest, PrimeReceiverPayloadTimeout) {
  std::vector<std::string> sub_frames = BuildSubFrames(
      std::string(1024, '\xaa'), GetMaxSubFrameSize(/*max_payload_size=*/32));

  Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
      /*ack=*/false, /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  frame = BuildPayloadFrame(2000, 0, sub_frames[0].substr(0, 30),
      /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
  ASSERT_TRUE(receiver_.receive_state().has_value());
  auto receive_state = receiver_.receive_state().value();
  ASSERT_EQ(receive_state.pieces.size(), 1);
  EXPECT_EQ(receive_state.pieces[0], sub_frames[0].substr(0, 30));

  clock_.SetTimeUs(kReceiverTimeoutUs + 1);
  frame = BuildPayloadFrame(2000, 0, sub_frames[0].substr(0, 30),
      /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
  EXPECT_FALSE(receiver_.receive_state().has_value());
}

TEST_F(RadioTransportReceiverTest, PrimeReceiverPartialPayloadEnd) {
  std::vector<std::string> sub_frames = BuildSubFrames(
      std::string(1024, '\xaa'), GetMaxSubFrameSize(/*max_payload_size=*/32));

  Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
      /*ack=*/false, /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  // Send frames 1, 3 and 9 into the receiver.
  frame = BuildPayloadFrame(2000, 0, sub_frames[0].substr(0, 30),
      /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
  frame = BuildPayloadFrame(2000, 3, sub_frames[0].substr(120, 30),
      /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
  frame = BuildPayloadFrame(2000, 9, sub_frames[0].substr(300, 30),
      /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  frame = BuildBeginEndFrame(2000, FrameType::END,
      /*ack=*/false, /*max_payload_size=*/32);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  // Set bits to ack sequence id 1, 3 and 9.
  frame = BuildBeginEndFrame(2000, FrameType::END,
      /*ack*/true, /*max_payload_size=*/32);
  frame.payload[2] = 0x09;
  frame.payload[3] = 0x02;
  EXPECT_EQ(transmit_count_, 2);
  EXPECT_EQ(last_send_frame_.address, frame.address);
  EXPECT_EQ(last_send_frame_.payload, frame.payload);
}

TEST_F(RadioTransportReceiverTest, PrimeReceiverPartialPayloadTruncatedHeader) {
  constexpr size_t kMaxPayloadSize = 8;
  max_payload_size_ = kMaxPayloadSize;

  std::vector<std::string> sub_frames = BuildSubFrames(
      std::string(1024, '\xaa'), GetMaxSubFrameSize(kMaxPayloadSize));

  Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
      /*ack=*/false, kMaxPayloadSize);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  // Send the first frame into the receiver.
  frame = BuildPayloadFrame(2000, 0, sub_frames[0].substr(0, 6),
      kMaxPayloadSize);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  frame = BuildBeginEndFrame(2000, FrameType::END,
      /*ack=*/false, kMaxPayloadSize);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  // Set bits to ack sequence id 1.
  frame = BuildBeginEndFrame(2000, FrameType::END,
      /*ack*/true, kMaxPayloadSize);
  frame.payload[2] = 0x01;
  EXPECT_EQ(transmit_count_, 2);
  EXPECT_EQ(last_send_frame_.address, frame.address);
  EXPECT_EQ(last_send_frame_.payload, frame.payload);
}

TEST_F(RadioTransportReceiverTest, PrimeReceiverPartialPayloadTruncatedFrame) {
  constexpr size_t kMaxPayloadSize = 32;
  max_payload_size_ = kMaxPayloadSize;

  std::vector<std::string> sub_frames = BuildSubFrames(
      std::string(1024, '\xaa'), GetMaxSubFrameSize(kMaxPayloadSize));

  Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
      /*ack=*/false, kMaxPayloadSize);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  // Send the first frame into the receiver.
  frame = BuildPayloadFrame(2000, 0,
      sub_frames[0].substr(0, kMaxPayloadSize - 2), kMaxPayloadSize);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  frame = BuildBeginEndFrame(2000, FrameType::END,
      /*ack=*/false, kMaxPayloadSize);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  // Set bits to ack sequence id 1.
  frame = BuildBeginEndFrame(2000, FrameType::END,
      /*ack*/true, kMaxPayloadSize);
  frame.payload[2] = 0x01;
  EXPECT_EQ(transmit_count_, 2);
  EXPECT_EQ(last_send_frame_.address, frame.address);
  EXPECT_EQ(last_send_frame_.payload, frame.payload);
}

TEST_F(RadioTransportReceiverTest, PrimeReceiverFirstSubFrame) {
  constexpr size_t kMaxPayloadSize = 8;
  max_payload_size_ = kMaxPayloadSize;

  std::string send_frame;
  for (size_t i = 0; i < 1024; i++) {
    send_frame.push_back(static_cast<char>(i));
  }

  std::vector<std::string> sub_frames = BuildSubFrames(
      send_frame, GetMaxSubFrameSize(kMaxPayloadSize));

  Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
      /*ack=*/false, kMaxPayloadSize);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  // Send the first sub frame into the receiver.
  uint8_t sequence_id = 0;
  for (size_t i = 0; i < sub_frames[0].size(); i += (kMaxPayloadSize - 2)) {
    frame = BuildPayloadFrame(2000, sequence_id++,
        sub_frames[0].substr(i, kMaxPayloadSize - 2), kMaxPayloadSize);
    EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
  }

  frame = BuildBeginEndFrame(2000, FrameType::END,
      /*ack=*/false, kMaxPayloadSize);
  EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

  // Set bits to ack all sequence IDs.
  frame = BuildBeginEndFrame(2000, FrameType::END,
      /*ack*/true, kMaxPayloadSize);
  frame.payload[2] = 0xff;
  frame.payload[3] = 0xff;
  frame.payload[4] = 0xff;
  frame.payload[5] = 0xff;
  frame.payload[6] = 0xff;
  frame.payload[7] = 0xff;
  EXPECT_EQ(transmit_count_, 2);
  EXPECT_EQ(last_send_frame_.address, frame.address);
  EXPECT_EQ(last_send_frame_.payload, frame.payload);

  auto receive_state = receiver_.receive_state().value();
  EXPECT_TRUE(receive_state.pieces.empty());
  EXPECT_EQ(receive_state.payload, send_frame.substr(0,
        GetMaxSubFrameSize(kMaxPayloadSize) - kPayloadHeaderSize));
}

TEST_F(RadioTransportReceiverTest, ReceiveFullPayloadCorruptedCRC) {
  constexpr size_t kMaxPayloadSize = 8;
  max_payload_size_ = kMaxPayloadSize;

  std::string send_frame;
  for (size_t i = 0; i < 1024; i++) {
    send_frame.push_back(static_cast<char>(i));
  }

  // Build sub frames and corrupt the CRC.
  std::vector<std::string> sub_frames = BuildSubFrames(
      send_frame, GetMaxSubFrameSize(kMaxPayloadSize));
  sub_frames.back()[sub_frames.back().size() - 2] = 0x00;

  // Send the sub frames into the receiver.
  for (const auto& sub_frame : sub_frames) {
    Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
        /*ack=*/false, kMaxPayloadSize);
    EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

    uint8_t sequence_id = 0;
    for (size_t i = 0; i < sub_frame.size(); i += (kMaxPayloadSize - 2)) {
      frame = BuildPayloadFrame(2000, sequence_id++,
          sub_frame.substr(i, kMaxPayloadSize - 2), kMaxPayloadSize);
      EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
    }

    frame = BuildBeginEndFrame(2000, FrameType::END,
        /*ack=*/false, kMaxPayloadSize);
    EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
  }

  EXPECT_EQ(transmit_count_, 8);
  EXPECT_FALSE(receiver_.receive_state().has_value());
}

TEST_F(RadioTransportReceiverTest, ReceiveFullPayload) {
  constexpr size_t kMaxPayloadSize = 8;
  max_payload_size_ = kMaxPayloadSize;

  std::string send_frame;
  for (size_t i = 0; i < 1024; i++) {
    send_frame.push_back(static_cast<char>(i));
  }

  // Build sub frames and send into the receiver.
  std::vector<std::string> sub_frames = BuildSubFrames(
      send_frame, GetMaxSubFrameSize(kMaxPayloadSize));

  size_t end_frame_count = 0;
  for (const auto& sub_frame : sub_frames) {
    Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
        /*ack=*/false, kMaxPayloadSize);
    EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

    uint8_t sequence_id = 0;
    for (size_t i = 0; i < sub_frame.size(); i += (kMaxPayloadSize - 2)) {
      frame = BuildPayloadFrame(2000, sequence_id++,
          sub_frame.substr(i, kMaxPayloadSize - 2), kMaxPayloadSize);
      EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
    }

    frame = BuildBeginEndFrame(2000, FrameType::END,
        /*ack=*/false, kMaxPayloadSize);
    if (++end_frame_count == 4) {
      auto payload = receiver_.HandleFrame(frame);
      ASSERT_TRUE(payload.has_value());
      EXPECT_EQ(payload.value(), send_frame);
    } else {
      EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
    }
  }

  EXPECT_EQ(transmit_count_, 8);
  EXPECT_FALSE(receiver_.receive_state().has_value());
}

TEST_F(RadioTransportReceiverTest, ReceiveFullPayloadLargerSingle) {
  constexpr size_t kMaxPayloadSize = 32;
  max_payload_size_ = kMaxPayloadSize;

  std::string send_frame;
  for (size_t i = 0; i < 512; i++) {
    send_frame.push_back(static_cast<char>(i));
  }

  // Build sub frames and send into the receiver.
  std::vector<std::string> sub_frames = BuildSubFrames(
      send_frame, GetMaxSubFrameSize(kMaxPayloadSize));

  size_t end_frame_count = 0;
  for (const auto& sub_frame : sub_frames) {
    Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
        /*ack=*/false, kMaxPayloadSize);
    EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

    uint8_t sequence_id = 0;
    for (size_t i = 0; i < sub_frame.size(); i += (kMaxPayloadSize - 2)) {
      frame = BuildPayloadFrame(2000, sequence_id++,
          sub_frame.substr(i, kMaxPayloadSize - 2), kMaxPayloadSize);
      EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
    }

    frame = BuildBeginEndFrame(2000, FrameType::END,
        /*ack=*/false, kMaxPayloadSize);
    if (++end_frame_count == 1) {
      auto payload = receiver_.HandleFrame(frame);
      ASSERT_TRUE(payload.has_value());
      EXPECT_EQ(payload.value(), send_frame);
    } else {
      EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
    }
  }

  EXPECT_EQ(transmit_count_, 2);
  EXPECT_FALSE(receiver_.receive_state().has_value());
}

TEST_F(RadioTransportReceiverTest, ReceiveFullPayloadLargerMulti) {
  constexpr size_t kMaxPayloadSize = 32;
  max_payload_size_ = kMaxPayloadSize;

  std::string send_frame;
  for (size_t i = 0; i < 8192; i++) {
    send_frame.push_back(static_cast<char>(i));
  }

  // Build sub frames and send into the receiver.
  std::vector<std::string> sub_frames = BuildSubFrames(
      send_frame, GetMaxSubFrameSize(kMaxPayloadSize));

  size_t end_frame_count = 0;
  for (const auto& sub_frame : sub_frames) {
    Link::Frame frame = BuildBeginEndFrame(2000, FrameType::BEGIN,
        /*ack=*/false, kMaxPayloadSize);
    EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);

    uint8_t sequence_id = 0;
    for (size_t i = 0; i < sub_frame.size(); i += (kMaxPayloadSize - 2)) {
      frame = BuildPayloadFrame(2000, sequence_id++,
          sub_frame.substr(i, kMaxPayloadSize - 2), kMaxPayloadSize);
      EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
    }

    frame = BuildBeginEndFrame(2000, FrameType::END,
        /*ack=*/false, kMaxPayloadSize);
    if (++end_frame_count == 2) {
      auto payload = receiver_.HandleFrame(frame);
      ASSERT_TRUE(payload.has_value());
      EXPECT_EQ(payload.value(), send_frame);
    } else {
      EXPECT_EQ(receiver_.HandleFrame(frame), std::nullopt);
    }
  }

  EXPECT_EQ(transmit_count_, 4);
  EXPECT_FALSE(receiver_.receive_state().has_value());
}

}  // namespace
}  // namespace nerfnet
