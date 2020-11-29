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

#ifndef NERFNET_NET_SECONDARY_RADIO_INTERFACE_H_
#define NERFNET_NET_SECONDARY_RADIO_INTERFACE_H_

#include "nerfnet/net/radio_interface.h"

namespace nerfnet {

// The secondary mode radio interface.
class SecondaryRadioInterface : public RadioInterface {
 public:
  // Setup the secondary radio link.
  SecondaryRadioInterface(uint16_t ce_pin, int tunnel_fd,
                          uint32_t primary_addr, uint32_t secondary_addr,
                          uint8_t channel);

  // Runs the interface listening for commands and responding.
  void Run();

 protected:
  // Set to true while a payload is in flight.
  bool payload_in_flight_;

  // Handles a request from the primary radio.
  void HandleRequest(const std::vector<uint8_t>& request);

  // Request handlers.
  void HandleNetworkTunnelReset();
  void HandleNetworkTunnelTxRx(const std::vector<uint8_t>& request);
};

}  // namespace nerfnet

#endif  // NERFNET_NET_SECONDARY_RADIO_INTERFACE_H_
