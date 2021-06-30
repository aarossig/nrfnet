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

#ifndef NERFNET_NET_TRANSPORT_MANAGER_H_
#define NERFNET_NET_TRANSPORT_MANAGER_H_

#include <memory>
#include <vector>

#include "nerfnet/net/transport.h"
#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// Manages an array of transports and forms the backbone of a nerfnet node.
class TransportManager : public NonCopyable,
                         public Transport::EventHandler {
 public:
  // Registers the transport with list of transports managed by this class.
  void RegisterTransport(Transport* transport);

  // Transport::EventHandler implementation.
  void OnBeaconFailed(Link::TransmitResult status) final;
  void OnBeaconReceived(uint32_t address) final;
  void OnFrameReceived(uint32_t address, const std::string& frame) final;

 private:
  // The transport instances registered with this transport manager.
  std::vector<Transport*> transports_;
};

}  // namespace nerfnet

#endif  // NERFNET_NET_TRANSPORT_MANAGER_H_
