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

#include <atomic>
#include <climits>
#include <condition_variable>
#include <thread>

#include "nerfnet/net/link.h"
#include "nerfnet/net/transport.h"

namespace nerfnet {

// A transport implementation that uses a radio as the underlying link. This
// transport breaks packets into smaller pieces to be transmitted with a radio.
class RadioTransport : public Transport {
 public:
  // The configuration to use for this radio transport.
  struct Config {
    // The interval between beacons in microseconds.
    uint64_t beacon_interval_us;
  };

  // The default config to use for the radio transport.
  static const Config kDefaultConfig;

  // Setup the transport with the link to use.
  RadioTransport(Link* link, EventHandler* event_handler,
      const Config& config = kDefaultConfig);

  // Stop the radio transport.
  virtual ~RadioTransport();

  // Transport implementation.
  SendResult Send(const std::string& frame, uint32_t address,
                  uint64_t timeout_us) final;

  // Returns the maximum sub-frame size.
  // Visible for testing.
  size_t GetMaxSubFrameSize() const;

 private:
  // The config to use for this transport.
  const Config config_;

  // The last time that a beacon was transmitted in microseconds.
  uint64_t last_beacon_time_us_;

  // The mutex used to guard access to the link.
  std::mutex link_mutex_;

  // The thread to use for sending beacons.
  std::atomic<bool> transport_running_;
  std::thread beacon_thread_;

  // The thread to use for sending/receiving frames.
  std::thread receive_thread_;

  // The thread to emit beacons on.
  void BeaconThread();

  // The thread to receive frames on. This allows continuously monitoring
  // for incoming packets and beacons to dispatch to the event handler.
  void ReceiveThread();

  // Builds a payload given a verb, sequence id and contents.
  std::string BuildPayload(uint8_t flags, uint8_t sequence_id,
      const std::string& contents);
};

}  // namespace nerfnet

#endif  // NERFNET_NET_RADIO_TRANSPORT_H_
