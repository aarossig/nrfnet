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

#include "nerfnet/net/link.h"
#include "nerfnet/net/nerfnet.pb.h"
#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// A transport over an abstract Link that permits sending larger data frames.
class Transport : public NonCopyable {
 public:
  // Setup a transport given a supplied link. The lifespan of the link must be
  // at least as long as this transport.
  Transport(Link* link);

  // The possible results of a send operation.
  enum class SendResult {
    // The frame was sent successfully.
    SUCCESS,

    // The frame could not be sent because it is invalid.
    INVALID_FRAME,
  };

  // Sends an arbitrary size data over the link.
  virtual SendResult Send(const NetworkFrame& frame, uint32_t address,
                          uint64_t timeout_us) = 0;

  // Returns the link associated with this transport.
  Link* link() const { return link_; }

 private:
  // The link to implement the transport over.
  Link* const link_;
};

}  // namespace nerfnet

#endif  // NERFNET_NET_TRANSPORT_H_
