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

#ifndef NERFNET_NET_RADIO_TRANSPORT_RECEIVER_
#define NERFNET_NET_RADIO_TRANSPORT_RECEIVER_

#include <map>
#include <string>

#include "nerfnet/net/link.h"
#include "nerfnet/util/non_copyable.h"
#include "nerfnet/util/time.h"

namespace nerfnet {

// The maximum amount of time to await a reply when sending/receiving a frame.
constexpr uint64_t kReceiveTimeoutUs = 10000;

// A class that accepts multiple link frames and assembles them into one larger
// payload.
class RadioTransportReceiver : public NonCopyable {
 public:
  // Setup the radio transport receiver with the clock to use. The clock must
  // have a duration that is at least as long as the lifespan of this object.
  RadioTransportReceiver(const Clock* clock);

  // Handles a link frame. Returns a full payload if one has been received.
  std::optional<std::string> HandleFrame(const Link::Frame& frame);

 private:
  // The clock to use for receiver timing.
  const Clock* const clock_;

  // State for the packet that is currently being received.
  struct ReceiveState {
    // The address of the node that packets are being accepted from.
    uint32_t address;

    // Received pieces of the current frame. These are assembled together and
    // appended to the frame below when all pieces have been received.
    std::map<uint8_t, std::string> frame_pieces_;

    // Entirely received portions of frames.
    std::string frame;

    // The timestamp of the last received packet for this frame.
    uint64_t receive_time_us;
  };

  // Contains the receive state of the current packet, if receiving one.
  std::optional<ReceiveState> receive_state_;

  // State for the last packet that was received to allow acknowledging the END
  // frame in the event that it is lost.
  struct LastReceiveState {
    // The address that the last frame came from.
    uint32_t address;

    // The maximum sequence ID for the previous frame.
    uint8_t max_sequence_id;

    // The time that the last frame was received.
    uint64_t receive_time_us;
  };

  // Contains the receive state for the last packet if received.
  std::optional<LastReceiveState> last_receive_state_;
};

}  // namespace nerfnet

#endif  // NERFNET_NET_RADIO_TRANSPORT_RECEIVER_
