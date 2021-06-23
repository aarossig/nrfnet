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

  // The thread to use for sending/receiving frames.
  std::atomic<bool> transport_thread_running_;
  std::thread transport_thread_;

  // Synchronization.
  std::mutex mutex_;
  std::condition_variable cv_;

  // A pointer to the frame to be sent, nullptr if there is currently no pending
  // frame. This is populated by the Send() method.
  std::string* send_frame_;

  // The thread to send/receive frames on. This allows continuously monitoring
  // for incoming packets and beacons to dispatch to the event handler.
  void TransportThread();
};

}  // namespace nerfnet

#endif  // NERFNET_NET_RADIO_TRANSPORT_H_
