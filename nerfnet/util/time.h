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

#ifndef NERFNET_UTIL_TIME_H_
#define NERFNET_UTIL_TIME_H_

#include <cstdint>

#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// Sleeps for the privided number of microseconds.
void SleepUs(uint64_t delay);

// Returns the current time in microseconds.
uint64_t TimeNowUs();

// An interface for a clock that provides time.
class Clock {
 public:
  virtual ~Clock() = default;

  // Returns the current time from this clock.
  virtual uint64_t TimeNowUs() const = 0;
};

// A clock that uses real system time to provide the current time.
class RealClock : public Clock,
                  public NonCopyable {
 public:
  // Clock implementation.
  uint64_t TimeNowUs() const final;
};

// A clock that uses a mock time to provide the current time.
class MockClock : public Clock,
                  public NonCopyable {
 public:
  // Setup the initial state of the mock clock.
  MockClock();

  // Clock implementation.
  uint64_t TimeNowUs() const;

  // Sets the current time of the clock.
  void SetTimeUs(uint64_t time_us);

 private:
  // The current time of the mocked clock.
  uint64_t time_us_;
};

}  // namespace nerfnet

#endif  // NERFNET_UTIL_TIME_H_
