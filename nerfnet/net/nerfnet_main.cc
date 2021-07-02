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

#include <tclap/CmdLine.h>

#include "nerfnet/net/network_manager.h"
#include "nerfnet/net/nrf_link.h"
#include "nerfnet/net/radio_transport.h"
#include "nerfnet/net/transport_manager.h"
#include "nerfnet/util/log.h"
#include "nerfnet/util/string.h"
#include "nerfnet/util/time.h"

// A description of the program.
constexpr char kDescription[] = "Mesh networking for NRF24L01 radios.";

// The version of the program.
constexpr char kVersion[] = "0.0.1";

int main(int argc, char** argv) {
  TCLAP::CmdLine cmd(kDescription, ' ', kVersion);
  TCLAP::ValueArg<uint32_t> address_arg("", "address",
      "The address of this station.", true, 0, "address", cmd);
  TCLAP::ValueArg<uint16_t> channel_arg("", "channel",
      "The channel to use for transmit/receive.", false, 1, "channel", cmd);
  TCLAP::ValueArg<uint16_t> ce_pin_arg("", "ce_pin",
      "Set to the index of the NRF24L01 chip-enable pin.", false, 22, "index",
      cmd);
  cmd.parse(argc, argv);


  // Register transports.
  std::unique_ptr<nerfnet::Link> link = std::make_unique<nerfnet::NRFLink>(
      address_arg.getValue(), channel_arg.getValue(), ce_pin_arg.getValue());

  // Setup the network.
  nerfnet::NetworkManager network_manager;
  network_manager.RegisterTransport<nerfnet::RadioTransport>(std::move(link));

  // TODO(aarossig): Start the network manager and block until quit.
  while (1) {
    nerfnet::SleepUs(1000000);
    LOGI("heartbeat");
  }

  return 0;
}
