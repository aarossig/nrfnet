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

#include "nerfnet/net/transport_manager.h"

#include "nerfnet/util/log.h"
#include "nerfnet/util/time.h"

namespace nerfnet {

TransportManager::RequestResult TransportManager::SendRequest(
    uint32_t address, uint64_t timeout_us,
    const Request& request, Response* response,
    const std::vector<uint32_t>& path) {
  NetworkFrame frame;
  frame.set_source_address(transport_->link()->address());
  *frame.mutable_destination_address() = { path.begin(), path.end() };
  *frame.mutable_request() = request;

  std::string serialized_frame;
  if (!frame.SerializeToString(&serialized_frame)) {
    LOGE("Failed to serialize frame");
    return RequestResult::FORMAT_ERROR;
  }

  std::unique_lock<std::mutex> lock(mutex_);
  std::unique_lock<std::mutex> response_lock(response_mutex_);
  response_address_ = address;
  response_.reset();
  uint64_t start_time_us = TimeNowUs();
  Transport::SendResult send_result =
      transport_->Send(serialized_frame, address, timeout_us);
  if (send_result != Transport::SendResult::SUCCESS) {
    LOGE("Failed to send frame: %d", send_result);
    return RequestResult::TRANSPORT_ERROR;
  }

  uint64_t time_now_us = TimeNowUs();
  if ((time_now_us - start_time_us) > timeout_us) {
    return RequestResult::TIMEOUT;
  }

  auto timeout = std::chrono::microseconds(time_now_us - start_time_us);
  response_cv_.wait_for(lock, timeout, [this]() {
    return response_.has_value();
  });

  if (!response_.has_value()) {
    return RequestResult::TIMEOUT;
  }

  *response = response_.value();
  return RequestResult::SUCCESS;
}

void TransportManager::OnBeaconFailed(Link::TransmitResult status) {
  LOGE("Failed to send beacon: %d", status);
}

void TransportManager::OnBeaconReceived(uint32_t address) {
  event_handler_->OnBeaconReceived(this, address);
}

void TransportManager::OnFrameReceived(
    uint32_t address, const std::string& frame) {
  NetworkFrame network_frame;
  if (network_frame.ParseFromString(frame)) {
    if (network_frame.destination_address().empty()) {
      if (!network_frame.has_source_address()) {
        LOGW("Ignoring network frame with missing source address");
      } else if (network_frame.has_request()) {
        event_handler_->OnRequest(this, address, network_frame.request());
      } else if (network_frame.has_response()) {
        std::unique_lock<std::mutex> lock(response_mutex_);
        if (response_address_ == network_frame.source_address()) {
          response_ = network_frame.response();
          response_cv_.notify_one();
        }
      }
    } else {
      std::unique_lock<std::mutex> lock(mesh_mutex_);
      mesh_frames_.push(network_frame);
      mesh_cv_.notify_one();
    }
  } else {
    LOGE("Failed to parse request from %u", address);
  }
}

void TransportManager::MeshThread() {
  // The timeout for mesh transmission operations.
  constexpr uint64_t kMeshTimeoutUs = 100000;

  while (mesh_running_) {
    std::unique_lock<std::mutex> lock(mesh_mutex_);
    mesh_cv_.wait(lock, [this]() {
      return !mesh_frames_.empty();
    });

    if (mesh_running_ && !mesh_frames_.empty()) {
      NetworkFrame frame = mesh_frames_.front();
      mesh_frames_.pop();

      uint32_t next_address = frame.destination_address()[0];
      frame.mutable_destination_address()->erase(0);

      std::string serialized_frame;
      if (frame.SerializeToString(&serialized_frame)) {
        LOGE("Failed to serialize frame");
      } else {
        std::unique_lock lock(mutex_);
        Transport::SendResult send_result = transport_->Send(
            serialized_frame, next_address, kMeshTimeoutUs);
        if (send_result != Transport::SendResult::SUCCESS) {
          LOGE("Failed to send frame: %d", send_result);
        }
      }
    }
  }
}

TransportManager::TransportManager()
    : mesh_running_(true),
      mesh_thread_(&TransportManager::MeshThread, this) {}

}  // namespace nerfnet
