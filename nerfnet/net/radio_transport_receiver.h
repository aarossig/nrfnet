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

#ifndef NERFNET_NET_RADIO_TRANSPORT_RECEIVER_
#define NERFNET_NET_RADIO_TRANSPORT_RECEIVER_

#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// A class that accepts multiple link frames and assembles them into one larger
// payload.
class RadioTransportReciever : public NonCopyable {
 public:
};

}  // namespace nerfnet

#endif  // NERFNET_NET_RADIO_TRANSPORT_RECEIVER_
