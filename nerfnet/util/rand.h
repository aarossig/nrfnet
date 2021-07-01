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

#ifndef NERFNET_UTIL_RAND_H_
#define NERFNET_UTIL_RAND_H_

#include <limits>
#include <random>

namespace nerfnet {

// Generate a random number in a given range.
template<typename T>
T Random(T min = std::numeric_limits<T>::min(),
         T max = std::numeric_limits<T>::max()) {
  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<T> distribution(min, max);
  return distribution(generator);
}

}  // namespace nerfnet

#endif  // NERFNET_UTIL_RAND_H_
