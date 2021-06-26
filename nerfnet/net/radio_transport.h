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

#ifndef NERFNET_NET_RADIO_TRANSPORT_H_
#define NERFNET_NET_RADIO_TRANSPORT_H_

#include <condition_variable>
#include <thread>

#include "nerfnet/net/link.h"
#include "nerfnet/net/radio_transport.pb.h"
#include "nerfnet/net/transport.h"

namespace nerfnet {

// A transport implementation that uses a radio as the underlying link. This
// transport breaks packets into smaller pieces to be transmitted with a radio.
class RadioTransport : public Transport {
 public:
  // Setup the transport with the link to use.
  RadioTransport(const RadioTransportConfig& config,
      Link* link, EventHandler* event_handler);

  // Stop the radio transport.
  virtual ~RadioTransport();

  // Transport implementation.
  SendResult Send(const NetworkFrame& frame, uint32_t address,
                  uint64_t timeout_us) final;

 private:
  // The config to use for this transport.
  const RadioTransportConfig config_;

  // The current mode of the transport.
  enum class Mode {
    // The transport is idle and currently awaiting reception of a message or
    // for a message to be queued to be transmitted.
    IDLE,

    // The transport is currently receiving a frame from another radio.
    RECEIVING,

    // The transport is currently transmitting a frame to another radio.
    SENDING,
  };

  // The current mode of the radio.
  Mode mode_;

  // The last time that a beacon was transmitted in microseconds.
  uint64_t last_beacon_time_us_;

  // Synchronization for sending/receiving frames.
  std::mutex mutex_;
  std::condition_variable cv_;

  // The mutex used to guard access to the link.
  std::mutex link_mutex_;

  // A pointer to the frame to be sent, nullptr if there is currently no pending
  // frame. This is populated by the Send() method.
  std::string* send_frame_;
  uint32_t send_address_;
  size_t send_frame_offset_;

  // The thread to use for sending beacons.
  std::atomic<bool> transport_running_;
  std::thread beacon_thread_;

  // The thread to use for sending/receiving frames.
  std::thread transport_thread_;

  // The thread to emit beacons on.
  void BeaconThread();

  // The thread to send/receive frames on. This allows continuously monitoring
  // for incoming packets and beacons to dispatch to the event handler.
  void TransportThread();

  // Sends a radio packet to another radio.
  uint64_t Send(uint64_t time_now_us);

  // Receive a radio packet from another radio.
  uint64_t Receive(uint64_t time_now_us);
};

}  // namespace nerfnet

#endif  // NERFNET_NET_RADIO_TRANSPORT_H_
