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

#include <gtest/gtest.h>

#include "nerfnet/net/mock_link.h"
#include "nerfnet/net/radio_transport.h"

namespace nerfnet {
namespace {

// A test fixture for the RadioTransport.
class RadioTransportTest : public ::testing::Test,
                           public Transport::EventHandler {
 protected:
  RadioTransportTest()
      : link_(/*address=*/1000, /*max_payload_size=*/32),
        transport_(RadioTransportConfig(), &link_, this) {}

  // Transport::EventHandler implementation.
  void OnBeaconReceived(uint32_t address) final {}

  // The transport instance to test and underlying mock link.
  MockLink link_;
  RadioTransport transport_;
};

TEST(RadioTransportTest, Foo) {
}

}  // anonymous namespace
}  // namespace nerfnet
