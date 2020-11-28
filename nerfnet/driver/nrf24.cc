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

#include "nerfnet/driver/nrf24.h"

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "nerfnet/util/log.h"
#include "nerfnet/util/string.h"

namespace nerfnet {

NRF24::NRF24(const std::string& spidev_path, uint16_t ce_pin, uint16_t cs_pin)
    : ce_pin_(ce_pin), cs_pin_(cs_pin) {
  SetupSPIDevice(spidev_path);
  InitGPIO(ce_pin);
}

NRF24::~NRF24() {
  close(spi_fd_);
}

void NRF24::SetupSPIDevice(const std::string& spidev_path) {
  // SPI bus configuration for NRF24L01.
  constexpr uint8_t kSpiMode = 0;
  constexpr uint8_t kSpiBitsPerWord = 8;
  constexpr uint32_t kSpiSpeedHz = 10000000;  // 10MHz.

  // Setup the SPI device.
  spi_fd_ = open(spidev_path.c_str(), O_RDWR);
  CHECK(spi_fd_ >= 0, "Failed to open spidev '%s' with: %s (%d)",
      spidev_path.c_str(), strerror(errno), errno);

  // Setup the SPI bus mode.
  int result = ioctl(spi_fd_, SPI_IOC_WR_MODE, &kSpiMode);
  CHECK(result >= 0, "Failed to set spidev write mode: %s (%d)",
      strerror(errno), errno);
  result = ioctl(spi_fd_, SPI_IOC_RD_MODE, &kSpiMode);
  CHECK(result >= 0, "Failed to set spidev read mode: %s (%d)",
      strerror(errno), errno);
  result = ioctl(spi_fd_, SPI_IOC_WR_BITS_PER_WORD, &kSpiBitsPerWord);
  CHECK(result >= 0, "Failed to set spidev write bits per word: %s (%d)",
      strerror(errno), errno);
  result = ioctl(spi_fd_, SPI_IOC_RD_BITS_PER_WORD, &kSpiBitsPerWord);
  CHECK(result >= 0, "Failed to set spidev read bits per word: %s (%d)",
      strerror(errno), errno);
  result = ioctl(spi_fd_, SPI_IOC_WR_MAX_SPEED_HZ, &kSpiSpeedHz);
  CHECK(result >= 0, "Failed to set spidev write speed: %s (%d)",
      strerror(errno), errno);
  result = ioctl(spi_fd_, SPI_IOC_RD_MAX_SPEED_HZ, &kSpiSpeedHz);
  CHECK(result >= 0, "Failed to set spidev read speed: %s (%d)",
      strerror(errno), errno);
}

void NRF24::InitGPIO(uint16_t pin_index) {
  int fd = open("/sys/class/gpio/export", O_WRONLY);
  CHECK(fd >= 0, "Failed to open GPIO export: %s (%d)",
      strerror(errno), errno);

  const auto pin_index_str = StringFormat("%d", pin_index);
  int result = write(fd, pin_index_str.c_str(), pin_index_str.size());
  CHECK(result == pin_index_str.size() || errno == EBUSY,
      "Failed to write GPIO export: %s (%d)",
      strerror(errno), errno);
  close(fd);

	const auto pin_direction_path = StringFormat(
      "/sys/class/gpio/gpio%d/direction", pin_index);
	fd = open(pin_direction_path.c_str(), O_WRONLY);
  CHECK(fd >= 0, "Failed to open GPIO direction '%s': %s (%d)",
      pin_direction_path.c_str(), strerror(errno), errno);

  const std::string kOutStr = "out";
  result = write(fd, kOutStr.c_str(), kOutStr.size());
  CHECK(result == kOutStr.size(),
      "Failed to write GPIO direction: %s (%d)",
      strerror(errno), errno);
  close(fd);
}

void NRF24::SetGPIO(uint16_t pin_index, bool value) {
  const auto pin_value_path = StringFormat(
	    "/sys/class/gpio/gpio%d/value", pin_index);
  int fd = open(pin_value_path.c_str(), O_WRONLY);
  CHECK(fd >= 0, "Failed to open GPIO value '%s': %s (%d)",
      pin_value_path.c_str(), strerror(errno), errno);
  int result = write(fd, value ? "1" : "0", 1);
  CHECK(result == 1, "Failed to write GPIO value '%s': %s (%d)",
      pin_value_path.c_str(), strerror(errno), errno);
	close(fd);
}

}  // namespace nerfnet
