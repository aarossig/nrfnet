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

#include "nerfnet/util/crc16.h"

namespace nerfnet {
namespace {

TEST(Crc16Test, EmptyBuffer) {
  EXPECT_EQ(GenerateCrc16(""), 0xffff);
}

TEST(Crc16Test, NonEmptyBuffer) {
  EXPECT_EQ(GenerateCrc16("Hello, World!"), 0x114e);
}

}  // namespace
}  // namespace nerfnet
