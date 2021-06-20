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

#ifndef NERFNET_NET_NRF_LINK_H_
#define NERFNET_NET_NRF_LINK_H_

#include <RF24/RF24.h>

#include "nerfnet/net/link.h"

namespace nerfnet {

// A Link implementation that uses an NRF24L01 radio.
class NRFLink : public Link {
 public:
  // Setup the link with the address and configuration for the NRF24L01 radio.
  NRFLink(uint32_t address, uint8_t channel, uint16_t ce_pin);

  // Link implementation.
  TransmitResult Beacon() final;
  ReceiveResult Receive(Frame* frame) final;
  TransmitResult Transmit(const Frame& frame) final;
  uint32_t GetMaxPayloadSize() const final;

 private:
  // The size of the frame to transmit with the NRF24L01 radio. The protocol
  // implemented here always transmits the full frame size of 32 bytes.
  static constexpr size_t kRawFrameSize = 32;

  // A typedef for a raw frame.
  typedef std::array<uint8_t, kRawFrameSize> RawFrame;

  // The possible states of the radio.
  enum RadioState {
    // The radio state is not known. This is the initial state until the first
    // call to send or receive.
    UNKNOWN,

    // The radio is currently in receiving mode.
    RECEIVING,

    // The radio is currently in transmitting mode.
    TRANSMITTING,
  };

  // The NRF24L01 radio driver instance.
  RF24 radio_;

  // The state of the radio.
  RadioState state_;

  // The last address that was transmitted to.
  uint32_t last_transmit_address_;

  // Fills in the address field of a raw frame.
  void PopulateAddress(RawFrame* raw_frame);

  // Puts the radio into receiving mode.
  void StartReceiving();

  // Puts the radio into transmitting mode and opens a writing pipe for the
  // given address.
  void StartTransmitting(uint32_t addresss);
};

}  // namespace nerfnet

#endif  // NERFNET_NET_NRF_LINK_H_
