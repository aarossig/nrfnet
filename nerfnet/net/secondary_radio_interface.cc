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

#include "nerfnet/net/secondary_radio_interface.h"

#include <vector>

#include "nerfnet/util/log.h"
#include "nerfnet/util/time.h"

namespace nerfnet {

SecondaryRadioInterface::SecondaryRadioInterface(
    uint16_t ce_pin, int tunnel_fd,
    uint32_t primary_addr, uint32_t secondary_addr)
    : RadioInterface(ce_pin, tunnel_fd, primary_addr, secondary_addr) {
  uint8_t writing_addr[5] = {
    static_cast<uint8_t>(secondary_addr),
    static_cast<uint8_t>(secondary_addr >> 8),
    static_cast<uint8_t>(secondary_addr >> 16),
    static_cast<uint8_t>(secondary_addr >> 24),
    0,
  };

  uint8_t reading_addr[5] = {
    static_cast<uint8_t>(primary_addr),
    static_cast<uint8_t>(primary_addr >> 8),
    static_cast<uint8_t>(primary_addr >> 16),
    static_cast<uint8_t>(primary_addr >> 24),
    0,
  };

  radio_.openWritingPipe(writing_addr);
  radio_.openReadingPipe(kPipeId, reading_addr);
  radio_.startListening();
}

void SecondaryRadioInterface::Run() {
  uint8_t packet[kMaxPacketSize];

  while (1) {
    if (radio_.available()) {
      radio_.read(&packet, sizeof(packet));

      LOGI("Received packet with length: %u", kMaxPacketSize);
      for (size_t i = 0; i < kMaxPacketSize; i++) {
        LOGI("Received byte %zu=%02x", i, packet[i]);
      }
    } else {
      SleepUs(kPollIntervalUs);
    }
  }
}

}  // namespace nerfnet
