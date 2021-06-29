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

#include "nerfnet/util/encode_decode.h"

#include "nerfnet/util/log.h"

#if !defined(__BYTE_ORDER__) || !defined(__ORDER_LITTLE_ENDIAN__)
#error "__BYTE_ORDER__ and __ORDER_LITTLE_ENDIAN__ must be defined"
#endif

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "This library is only compatible with little-endian machines"
#endif

namespace nerfnet {

std::string EncodeU32(uint32_t value) {
  std::string encoded_value;
  encoded_value.push_back(static_cast<uint8_t>(value));
  encoded_value.push_back(static_cast<uint8_t>(value >> 8));
  encoded_value.push_back(static_cast<uint8_t>(value >> 16));
  encoded_value.push_back(static_cast<uint8_t>(value >> 24));
  return encoded_value;
}

std::string EncodeU16(uint16_t value) {
  std::string encoded_value;
  encoded_value.push_back(static_cast<uint8_t>(value));
  encoded_value.push_back(static_cast<uint8_t>(value >> 8));
  return encoded_value;
}

uint32_t DecodeU32(const std::string_view& str) {
  CHECK(str.size() >= sizeof(uint32_t),
      "Unable to decode string with size %zu vs expected %zu",
      str.size(), sizeof(uint32_t));
  return static_cast<uint32_t>(str[0])
      | (static_cast<uint32_t>(str[1]) << 8)
      | (static_cast<uint32_t>(str[2]) << 16)
      | (static_cast<uint32_t>(str[3]) << 24);
}

uint16_t DecodeU16(const std::string_view& str) {
  CHECK(str.size() >= sizeof(uint16_t),
      "Unable to decode string with size %zu vs expected %zu",
      str.size(), sizeof(uint16_t));
  return static_cast<uint16_t>(str[0])
      | (static_cast<uint16_t>(str[1]) << 8);
}

}  // namespace nerfnet
