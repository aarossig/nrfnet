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

#include "nerfnet/net/nrf_link.h"
#include "nerfnet/util/log.h"
#include "nerfnet/util/string.h"
#include "nerfnet/util/time.h"

// A description of the program.
constexpr char kDescription[] = "Mesh networking for NRF24L01 radios.";

// The version of the program.
constexpr char kVersion[] = "0.0.1";

using nerfnet::Link;
using nerfnet::NRFLink;

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

  NRFLink link(address_arg.getValue(), channel_arg.getValue(),
      ce_pin_arg.getValue());

  constexpr uint64_t kBeaconIntervalUs = 200000;
  constexpr uint64_t kDataIntervalUs = 1000000;
  uint64_t last_beacon_time_us = 0;
  uint64_t last_data_time_us = 0;
  while (1) {
    uint64_t time_now_us = nerfnet::TimeNowUs();
    if ((time_now_us - last_beacon_time_us) > kBeaconIntervalUs) {
      LOGI("beaconing");
      Link::TransmitResult result = link.Beacon();
      if (result != Link::TransmitResult::SUCCESS) {
        LOGE("Beacon failed: %d", result);
      }

      last_beacon_time_us = time_now_us;
    } else if ((time_now_us - last_data_time_us) > kDataIntervalUs) {
      LOGI("data");
      Link::Frame frame;
      frame.address = 2000;
      frame.payload = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
      Link::TransmitResult result = link.Transmit(frame);
      if (result != Link::TransmitResult::SUCCESS) {
        LOGE("Transmit failed: %d", result);
      }

      last_data_time_us = time_now_us;
    } else {
      Link::Frame frame;
      Link::ReceiveResult result = link.Receive(&frame);
      if (result == Link::ReceiveResult::NOT_READY) {
        nerfnet::SleepUs(10000);
      } else {
        if (result == Link::ReceiveResult::SUCCESS) {
          std::string frame_contents_str;
          for (const auto& byte : frame.payload) {
            frame_contents_str += nerfnet::StringFormat("0x%02x ", byte);
          }

          LOGI("Received frame: address=%" PRIu32 ", size=%zu, contents='%s'",
              frame.address, frame.payload.size(), frame_contents_str.c_str());
        } else {
          LOGE("Receive failed: %d", result);
        }
      }
    }
  }

  return 0;
}
