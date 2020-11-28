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

#ifndef NERFNET_DRIVER_NRF24_H_
#define NERFNET_DRIVER_NRF24_H_

#include <string>

#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// A spidev-based driver for the NRF24L01 chipset.
class NRF24 : public NonCopyable {
 public:
  // Setup the NRF24 device.
  NRF24(const std::string& spidev_path, uint16_t ce_pin, uint16_t cs_pin);

  // Cleanup resources associated with this device.
  ~NRF24();

 private:
  // GPIOs for chip-select and chip-enable.
  const uint16_t ce_pin_;
  const uint16_t cs_pin_;

  // The file descriptor for the spidev device used to perform SPI
  // transactions.
  int spi_fd_;

  // Setup the SPI device for the supplied path. All errors are fatal and
  // logged.
  void SetupSPIDevice(const std::string& spidev_path);

  // Export and configure the supplied GPIO.
  void InitGPIO(uint16_t pin_index);

  // Set the value of the supplied GPIO.
  void SetGPIO(uint16_t pin_index, bool value);
};

}  // namespace nerfnet

#endif  // NERFNET_DRIVER_NRF24_H_
