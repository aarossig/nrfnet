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

#include "nerfnet/net/radio_interface.h"

#include "nerfnet/util/log.h"

namespace nerfnet {

RadioInterface::RadioInterface(uint16_t ce_pin, int tunnel_fd,
                               uint32_t primary_addr, uint32_t secondary_addr)
    : radio_(ce_pin, 0),
      tunnel_fd_(tunnel_fd),
      primary_addr_(primary_addr),
      secondary_addr_(secondary_addr) {
  radio_.begin();
  radio_.setPALevel(RF24_PA_MAX);
  radio_.setDataRate(RF24_1MBPS);
  radio_.setAutoAck(1);
  radio_.setRetries(2, 15);
  radio_.setCRCLength(RF24_CRC_8);
  radio_.enableAckPayload();
  radio_.enableDynamicPayloads();
  CHECK(radio_.isChipConnected(), "NRF24L01 is unavailable");
}

}  // namespace nerfnet
