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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "nerfnet/net/nerfnet.pb.h"
#include "nerfnet/net/transport.h"
#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// Manages an array of transports and forms the backbone of a nerfnet node.
class TransportManager : public NonCopyable,
                         public Transport::EventHandler {
 public:
  // The event handler for this transport manager.
  class EventHandler {
   public:
    virtual ~EventHandler() = default;

    // Invoked when a beacon is received.
    virtual void OnBeaconReceived(TransportManager* transport_manager,
        uint32_t address) = 0;

    // Invoked when a request is received. This can be called on an arbitrary
    // thread so appropriate locks must be held.
    virtual void OnRequest(TransportManager* transport_manager,
        uint32_t address, const Request& request) = 0;
  };

  // Create a TransportManager that owns a link and transport.
  template<typename TransportType, typename... Args>
  static std::unique_ptr<TransportManager> Create(std::unique_ptr<Link> link,
      EventHandler* event_handler, Args&&... args) {
    std::unique_ptr<TransportManager> transport_manager(new TransportManager());
    transport_manager->event_handler_ = event_handler;
    transport_manager->link_ = std::move(link);
    transport_manager->transport_ = std::make_unique<TransportType>(
        transport_manager->link_.get(), transport_manager.get(),
            std::forward<Args>(args)...);
    return transport_manager;
  }

  // The possible results of a send operation.
  enum class RequestResult {
    // The request was made successfully.
    SUCCESS,

    // The request timed out.
    TIMEOUT,

    // There was an error formatting the request.
    FORMAT_ERROR,

    // There was an error sending the request over the transport.
    TRANSPORT_ERROR,
  };

  // Sends a request to the given address. If successful, the response is
  // populated.
  RequestResult SendRequest(uint32_t address, uint64_t timeout_us,
      const Request& request, Response* response,
      const std::vector<uint32_t>& path = {});

  // Transport::EventHandler implementation.
  void OnBeaconFailed(Link::TransmitResult status) final;
  void OnBeaconReceived(uint32_t address) final;
  void OnFrameReceived(uint32_t address, const std::string& frame) final;

 private:
  // The event handler to dispatch events to.
  EventHandler* event_handler_;

  // The transport and link instances managed here.
  std::unique_ptr<Link> link_;
  std::unique_ptr<Transport> transport_;

  // Synchronization for sending.
  std::mutex mutex_;

  // Synchronization for request/response.
  std::mutex response_mutex_;
  std::condition_variable response_cv_;
  uint32_t response_address_;
  std::optional<Response> response_;

  // The queue of mesh messages to forward.
  std::mutex mesh_mutex_;
  std::condition_variable mesh_cv_;
  std::queue<NetworkFrame> mesh_frames_;
  std::atomic<bool> mesh_running_;
  std::thread mesh_thread_;

  // The thread that forwards mesh requests.
  void MeshThread();

  // Mark the default constructor private. This class should be instantiated
  // with the `Create` static factory method.
  TransportManager();
};

}  // namespace nerfnet

#endif  // NERFNET_NET_TRANSPORT_MANAGER_H_
