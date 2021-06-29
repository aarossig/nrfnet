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

#include "nerfnet/util/log.h"
#include "nerfnet/util/encode_decode.h"

namespace nerfnet {
namespace {

TEST(EncodeDecodeTest, EncodeU32) {
  EXPECT_EQ(EncodeU32(0xdeadbeef), "\xef\xbe\xad\xde");
}

TEST(EncodeDecodeTest, EncodeU16) {
  EXPECT_EQ(EncodeU16(0xdead), "\xad\xde");
}

TEST(EncodeDecodeTest, Decode32) {
  EXPECT_EQ(DecodeU32("\xef\xbe\xad\xde"), 0xdeadbeef);
}

TEST(EncodeDecodeTest, Decode16) {
  EXPECT_EQ(DecodeU16("\xef\xbe"), 0xbeef);
}

}  // anonymous namespace
}  // namespace nerfnet
