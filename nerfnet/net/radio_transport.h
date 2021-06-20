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

#ifndef NERFNET_NET_RADIO_TRANSPORT_H_
#define NERFNET_NET_RADIO_TRANSPORT_H_

#include "nerfnet/net/radio_interface.h"
#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// A transport over an abstract RadioInterface that permits sending larger
// data frames.
class RadioTransport : public NonCopyable {
 public:
  // Setup a radio transport given a supplied radio interface. The lifespan of
  // the radio interface must be at least as long as this transport.
  RadioTransport(RadioInterface* radio_interface);

 private:
  // The radio interface to implement the transport over.
  RadioInterface* const radio_interface_;
};

}  // namespace nerfnet

#endif  // NERFNET_NET_RADIO_TRANSPORT_H_
