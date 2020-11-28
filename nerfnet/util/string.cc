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

#include "nerfnet/util/string.h"

#include <cstdarg>

#include "nerfnet/util/log.h"

namespace nerfnet {

std::string StringFormat(const char* format, ...) {
  va_list vl, vl_copy;
  va_start(vl, format);
  va_copy(vl_copy, vl);

  int size = vsnprintf(nullptr, 0, format, vl);
  CHECK(size >= 0, "Failed to determine output size");

  std::string output = std::string(size + 1, '\0');
  size = vsnprintf(output.data(), output.size(), format, vl_copy);
  CHECK(size >= 0, "Failed to format outout");

  va_end(vl_copy);
  va_end(vl);
  return output;
}

}  // namespace nerfnet
