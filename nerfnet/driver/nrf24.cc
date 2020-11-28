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

namespace {

// SPI bus configuration for NRF24L01.
const uint8_t kSpiMode = 0;
const uint8_t kSpiBitsPerWord = 8;
const uint32_t kSpiSpeedHz = 10000000;  // 10MHz.

}  // anonymous namespace

NRF24::NRF24(const std::string& spidev_path, uint16_t ce_pin)
    : ce_pin_(ce_pin) {
  SetupSPIDevice(spidev_path);
  InitChipEnable();
  SetChipEnable(false);


  uint8_t value = 0x23;
  WriteRegister(0x00, value);
  value = 0x00;
  ReadRegister(0x00, &value);
  LOGI("value: 0x%02x", value);
}

NRF24::~NRF24() {
  close(spi_fd_);
}

void NRF24::WriteRegister(uint8_t address, uint8_t value) {
  WriteRegister(address, std::vector<uint8_t>({value}));
}

void NRF24::WriteRegister(uint8_t address, const std::vector<uint8_t>& value) {
  std::vector<uint8_t> command;
  command.push_back(0x20 | address);
  command.insert(command.end(), value.begin(), value.end());
  std::vector<uint8_t> response;
  PerformSPITransaction(command, response);
}

void NRF24::ReadRegister(uint8_t address, uint8_t* value) {
  std::vector<uint8_t> response_value;
  response_value.resize(1, 0x00);
  ReadRegister(address, response_value);
  *value = response_value[0];
}

void NRF24::ReadRegister(uint8_t address, std::vector<uint8_t>& value) {
  std::vector<uint8_t> command;
  command.resize(1 + value.size(), 0x00);
  command[0] = address;
  std::vector<uint8_t> response;
  PerformSPITransaction(command, response);
  value = {response.begin() + 1, response.end()};
}

void NRF24::SetupSPIDevice(const std::string& spidev_path) {
  // Setup the SPI device.
  spi_fd_ = open(spidev_path.c_str(), O_RDWR);
  CHECK(spi_fd_ >= 0, "Failed to open spidev '%s' with: %s (%d)",
      spidev_path.c_str(), strerror(errno), errno);

// SPI bus configuration for NRF24L01.
const uint8_t kSpiMode = 0;
const uint8_t kSpiBitsPerWord = 8;
const uint32_t kSpiSpeedHz = 10000000;  // 10MHz.


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

void NRF24::PerformSPITransaction(const std::vector<uint8_t>& tx_buffer) {
  std::vector<uint8_t> response;
  PerformSPITransaction(tx_buffer, response);
}

void NRF24::PerformSPITransaction(const std::vector<uint8_t>& tx_buffer,
                                  std::vector<uint8_t>& rx_buffer) {
  rx_buffer.clear();
  rx_buffer.resize(tx_buffer.size());

  struct spi_ioc_transfer transfer = {};
  transfer.tx_buf = reinterpret_cast<uint64_t>(tx_buffer.data());
  transfer.rx_buf = reinterpret_cast<uint64_t>(rx_buffer.data());
  transfer.len = tx_buffer.size();
  transfer.speed_hz = kSpiSpeedHz;
  transfer.delay_usecs = 0;
  transfer.bits_per_word = kSpiBitsPerWord;
  transfer.cs_change = 0;
  
  int result = ioctl(spi_fd_, SPI_IOC_MESSAGE(1), &transfer);
  CHECK(result >= 0, "Failed to perform SPI transaction: %s (%d)",
      strerror(errno), errno);
}

void NRF24::InitChipEnable() {
  int fd = open("/sys/class/gpio/export", O_WRONLY);
  CHECK(fd >= 0, "Failed to open GPIO export: %s (%d)",
      strerror(errno), errno);

  const auto ce_pin_str = StringFormat("%d", ce_pin_);
  int result = write(fd, ce_pin_str.c_str(), ce_pin_str.size());
  CHECK(result == ce_pin_str.size() || errno == EBUSY,
      "Failed to write GPIO export: %s (%d)",
      strerror(errno), errno);
  close(fd);

	const auto ce_pin_direction_path = StringFormat(
      "/sys/class/gpio/gpio%d/direction", ce_pin_);
	fd = open(ce_pin_direction_path.c_str(), O_WRONLY);
  CHECK(fd >= 0, "Failed to open GPIO direction '%s': %s (%d)",
      ce_pin_direction_path.c_str(), strerror(errno), errno);

  const std::string kOutStr = "out";
  result = write(fd, kOutStr.c_str(), kOutStr.size());
  CHECK(result == kOutStr.size(),
      "Failed to write GPIO direction: %s (%d)",
      strerror(errno), errno);
  close(fd);
}

void NRF24::SetChipEnable(bool value) {
  const auto ce_value_path = StringFormat(
	    "/sys/class/gpio/gpio%d/value", ce_pin_);
  int fd = open(ce_value_path.c_str(), O_WRONLY);
  CHECK(fd >= 0, "Failed to open GPIO value '%s': %s (%d)",
      ce_value_path.c_str(), strerror(errno), errno);
  int result = write(fd, value ? "1" : "0", 1);
  CHECK(result == 1, "Failed to write GPIO value '%s': %s (%d)",
      ce_value_path.c_str(), strerror(errno), errno);
	close(fd);
}

}  // namespace nerfnet
