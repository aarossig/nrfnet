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

#ifndef NERFNET_NET_NETWORK_MANAGER_H_
#define NERFNET_NET_NETWORK_MANAGER_H_

#include "nerfnet/net/transport_manager.h"
#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// The top-level class that manages all transports, beaconing and tunnelling.
class NetworkManager : public NonCopyable,
                       public TransportManager::EventHandler {
 public:
  // Registers a transport with the network.
  template<typename T, typename... Args>
  void RegisterTransport(std::unique_ptr<Link> link) {
    std::unique_ptr<TransportManager> transport =
        TransportManager::Create<T>(std::move(link), this);
    transports_.emplace_back();
    transports_.back().transport = std::move(transport);
  }

  // TransportManager::EventHandler implementation.
  void OnBeaconReceived(TransportManager* transport_manager,
      uint32_t address) final;
  void OnRequest(TransportManager* transport_manager,
      uint32_t address, const Request& request) final;

 private:
  // State relating to each transport manager.
  struct TransportManagerContext {
    std::unique_ptr<TransportManager> transport;
  };

  // The transports managed by this network.
  std::vector<TransportManagerContext> transports_;
};

}  // namespace nerfnet

#endif  // NERFNET_NET_NETWORK_MANAGER_H_
