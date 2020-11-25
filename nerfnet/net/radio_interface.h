/*
 * Copyright 2020 Andrew Rossignol andrew.rossignol@gmail.com
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

#ifndef NERFNET_NET_RADIO_INTERFACE_H_
#define NERFNET_NET_RADIO_INTERFACE_H_

#include <deque>
#include <RF24/RF24.h>
#include <thread>

#include "nerfnet/net/net.pb.h"
#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// The interface to send/receive data using an RF24 radio.
class RadioInterface : public NonCopyable {
 public:
  // Setup the radio interface.
  RadioInterface(uint16_t ce_pin, int tunnel_fd,
                 uint32_t primary_addr, uint32_t secondary_addr,
                 uint64_t rf_delay_us);
  ~RadioInterface();

  // The possible results of a request operation.
  enum class RequestResult {
    // The request was successful.
    Success,

    // The request timed out.
    Timeout,

    // The request could not be sent because it was malformed.
    Malformed,

    // There was an error transmitting the request.
    TransmitError,
  };

 protected:
  // The number of microseconds to poll over.
  static constexpr uint32_t kPollIntervalUs = 1000;

  // The maximum size of a packet.
  static constexpr size_t kMaxPacketSize = 32;

  // The default pipe to use for sending data.
  static constexpr uint8_t kPipeId = 1;

  // The underlying radio.
  RF24 radio_;

  // The file descriptor for the network tunnel.
  const int tunnel_fd_;

  // The addresses to use for this radio pair.
  const uint32_t primary_addr_;
  const uint32_t secondary_addr_;

  // The delay to use between RF operation.s
  const uint64_t rf_delay_us_;

  // The thread to read from the tunnel interface on.
  std::thread tunnel_thread_;
  std::atomic<bool> running_;

  // The buffer of data read and lock.
  std::mutex read_buffer_mutex_;
  std::deque<std::vector<uint8_t>> read_buffer_;

  // The frame buffer for the currently incoming frame. Written out to
  // the tunnel interface when completely received.
  std::string frame_buffer_;

  // Sends a message over the radio.
  RequestResult Send(const google::protobuf::Message& request);

  // Reads a message from the radio.
  RequestResult Receive(google::protobuf::Message& response,
      uint64_t timeout_us = 0);

  // Reads from the tunnel and buffers data read.
  void TunnelThread();
};

}  // namespace nerfnet

#endif  // NERFNET_NET_RADIO_INTERFACE_H_
