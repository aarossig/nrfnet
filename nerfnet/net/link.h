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

#ifndef NERFNET_NET_LINK_H_
#define NERFNET_NET_LINK_H_

#include <cstdint>
#include <string>

#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// The radio interface to send/receive packets over.
class Link : public NonCopyable {
 public:
  // Setup the radio interface with the address of this station.
  Link(uint32_t address);

  // Provide a virtual destructor for subclasses.
  virtual ~Link() = default;

  // The result of a transmit operation. Used for sending and beacon operations.
  enum class TransmitResult {
    // The transmission was successful.
    SUCCESS,

    // The supplied frame is too large to transmit on this radio.
    TOO_LARGE,

    // There was an error transmitting the frame.
    TRANSMIT_ERROR,
  };

  // Emit a beacon packet for this station.
  virtual TransmitResult Beacon() = 0;

  // The result of a receive operation. Used for receiving packets which may
  // ether be data frames or beacon frames.
  enum class ReceiveResult {
    // A frame was received successfully.
    SUCCESS,

    // There was no frame ready.
    NOT_READY,

    // There was an error receiving the frame.
    RECEIVE_ERROR,
  };

  // A frame to send/receive with the radio.
  struct Frame {
    // The address of the other station or this station depending on whether
    // transmitting or receiving.
    uint32_t address = 0;

    // The payload of the frame. If this is empty, then the frame is a beacon
    // frame that contains no content.
    std::string payload;
  };

  // Receives a single frame from the radio, populating the address and payload
  // contents if successful. Returns true if a frame was received.
  virtual ReceiveResult Receive(Frame* frame) = 0;

  // Transmits the supplied frame.
  virtual TransmitResult Transmit(const Frame& frame) = 0;

  // Returns the maximum size of payload for a given frame that can be
  // transmitted.
  virtual uint32_t GetMaxPayloadSize() const = 0;

  // Returns the address of this node.
  uint32_t address() const { return address_; }

 private:
  // The address of this station.
  uint32_t address_;
};

}  // namespace nerfnet

#endif  // NERFNET_NET_LINK_H_
