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

#include "nerfnet/util/file.h"
#include "nerfnet/util/file_test.pb.h"

namespace nerfnet {
namespace {

TEST(File, InvalidFile) {
  std::string file_contents;
  EXPECT_FALSE(ReadFileToString("invalid_filename_lol.txt", &file_contents));
}

TEST(File, ValidFile) {
  std::string file_contents;
  EXPECT_TRUE(ReadFileToString("nerfnet/util/testdata/file.txt", &file_contents));
  EXPECT_EQ(file_contents, "file\n");
}

TEST(File, TextProtoFile) {
  FileTest file_test;
  EXPECT_TRUE(ReadTextProtoFileToMessage("nerfnet/util/testdata/file.textproto",
        &file_test));
  EXPECT_TRUE(file_test.has_greeting());
  EXPECT_EQ(file_test.greeting(), "Hello, World!");
}

}  // anonymous namespace
}  // namespace nerfnet
