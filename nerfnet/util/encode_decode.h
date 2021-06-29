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

#ifndef NERFNET_UTIL_ENCODE_DECODE_H_
#define NERFNET_UTIL_ENCODE_DECODE_H_

#include <string>

namespace nerfnet {

// Encodes a uint32_t value as a little-endian string.
std::string EncodeU32(uint32_t value);

// Encodes a uint16_t as a little-endian string.
std::string EncodeU16(uint16_t value);

// Decodes a uint32_t value from a little-endian string. The string must be at
// least 4 bytes long.
uint32_t DecodeU32(const std::string_view& str);

// Deecodes a uint16_t value from a little-endian string. The string must be at
// least 2 bytes long.
uint16_t DecodeU16(const std::string_view& str);

}  // namespace nerfnet

#endif  // NERFNET_UTIL_ENCODE_DECODE_H_
