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

#include "nerfnet/util/encode_decode.h"

namespace nerfnet {
namespace {

TEST(EncodeDecodeTest, EncodeUnsigned32BitInteger) {
  EXPECT_EQ(EncodeValue(0xdeadbeef), "\xef\xbe\xad\xde");
}

}  // anonymous namespace
}  // namespace nerfnet
