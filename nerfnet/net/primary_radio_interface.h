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

#ifndef NERFNET_NET_PRIMARY_RADIO_INTERFACE_H_
#define NERFNET_NET_PRIMARY_RADIO_INTERFACE_H_

#include <optional>

#include "nerfnet/net/radio_interface.h"

namespace nerfnet {

// The primary mode radio interface.
class PrimaryRadioInterface : public RadioInterface {
 public:
  // Setup the primary radio link.
  PrimaryRadioInterface(uint16_t ce_pin, int tunnel_fd,
                        uint32_t primary_addr, uint32_t secondary_addr,
                        uint8_t channel, uint64_t poll_interval_us);

  // Runs the interface.
  void Run();

 private:
  // The interval between poll operations to the secondary radio.
  const uint64_t poll_interval_us_;

  // Logic for poll backoff when the secondary radio is not responding.
  int poll_fail_count_;
  uint64_t current_poll_interval_us_;
  bool connection_reset_required_;

  // Requests that a new connection be opened.
  bool ConnectionReset();

  // Sends and receives messages to exchange network packets.
  bool PerformTunnelTransfer();

  // Updates the backoff configuration in the light of a failure.
  void HandleTransactionFailure();

};

}  // namespace nerfnet

#endif  // NERFNET_NET_PRIMARY_RADIO_INTERFACE_H_
