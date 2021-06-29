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

#ifndef NERFNET_NET_TRANSPORT_H_
#define NERFNET_NET_TRANSPORT_H_

#include <string>

#include "nerfnet/net/link.h"
#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// A transport over an abstract Link that permits sending larger data frames.
class Transport : public NonCopyable {
 public:
  // The event handler for this transport.
  class EventHandler {
   public:
    virtual ~EventHandler() = default;

    // Called when a beacon transmission fails. This provides the implementation
    // with the status of the transmission that triggered the failure.
    virtual void OnBeaconFailed(Link::TransmitResult status) = 0;

    // Called when a beacon is received. Beacons may be received at any time.
    // This method is called on an internal thread so appropriate locks must be
    // held.
    virtual void OnBeaconReceived(uint32_t address) = 0;

    // Called when a frame is received. Frames may be received at any time. This
    // method is called on an internal thread so appropriate locks must be held.
    virtual void OnFrameReceived(uint32_t address,
        const std::string& frame) = 0;
  };

  // Setup a transport given a supplied link. The lifespan of the link must be
  // at least as long as this transport.
  Transport(Link* link, EventHandler* event_handler);

  // Provide a virtual destructor for subclasses.
  virtual ~Transport() = default;

  // The possible results of a send operation.
  enum class SendResult {
    // The frame was sent successfully.
    SUCCESS,

    // The frame transmission deadline was exceeded.
    TIMEOUT,

    // There was an error sending this frame over the link.
    TRANSMIT_ERROR,

    // There was an error receiving a response to this frame.
    RECEIVE_ERROR,
  };

  // Sends an arbitrary size data over the link.
  virtual SendResult Send(const std::string& frame, uint32_t address,
                          uint64_t timeout_us) = 0;

  // Returns the link associated with this transport.
  Link* link() const { return link_; }

  // Returns the event handler associated with this transport.
  EventHandler* event_handler() const { return event_handler_; }

 private:
  // The link to implement the transport over.
  Link* const link_;

  // The event handler for this transport.
  EventHandler* const event_handler_;
};

}  // namespace nerfnet

#endif  // NERFNET_NET_TRANSPORT_H_
