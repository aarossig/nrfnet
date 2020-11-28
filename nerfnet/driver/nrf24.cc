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
constexpr uint8_t kSpiMode = 0;
constexpr uint8_t kSpiBitsPerWord = 8;
constexpr uint32_t kSpiSpeedHz = 10000000;  // 10MHz.

// Register definitions.
constexpr uint8_t kRegisterConfig = 0x00;
constexpr uint8_t kRegisterAutoAck = 0x01;
constexpr uint8_t kRegisterReceiveAddress = 0x02;
constexpr uint8_t kRegisterAddressWidth = 0x03;
constexpr uint8_t kRegisterChannel = 0x05;
constexpr uint8_t kRegisterRFConfig = 0x06;
constexpr uint8_t kRegisterRxAddressBase = 0x0a;
constexpr uint8_t kRegisterTxAddress = 0x10;

}  // anonymous namespace

NRF24::NRF24(const std::string& spidev_path, uint16_t ce_pin)
    : ce_pin_(ce_pin),
      channel_(0),
      power_level_(PowerLevel::High),
      data_rate_(DataRate::R1MBPS),
      address_width_(3),
      auto_ack_enabled_(true),
      crc_mode_(CRCMode::C16Bit),
      in_receive_mode_(true) {
  SetupSPIDevice(spidev_path);
  InitChipEnable();
  SetChipEnable(false);
  CHECK(WriteConfig(), "Failed to write config on startup");
}

NRF24::~NRF24() {
  close(spi_fd_);
}

bool NRF24::SetChannel(uint8_t channel) {
  if (channel > 127) {
    return false;
  }

  channel_ = channel;
  return true;
}

bool NRF24::SetPowerLevel(PowerLevel power_level) {
  switch (power_level) {
    case PowerLevel::Min:
    case PowerLevel::Med:
    case PowerLevel::High:
    case PowerLevel::Max:
      power_level_ = power_level;
      return true;
  }

  return false;
}

bool NRF24::SetDataRate(DataRate data_rate) {
  switch (data_rate) {
    case DataRate::R1MBPS:
    case DataRate::R2MBPS:
    case DataRate::R250KBPS:
      data_rate_ = data_rate;
      return true;
  }

  return false;
}

bool NRF24::SetAddressWidth(size_t address_width) {
  if (address_width < 3 || address_width > 5) {
    return false;
  }

  address_width_ = address_width;
  return true;
}

bool NRF24::SetTransmitAddress(const std::vector<uint8_t>& address) {
  if (!ValidateAddress(address)) {
    return false;
  }

  tx_address_ = address;
  return true;
}

bool NRF24::SetReceiveAddress(size_t id, const std::vector<uint8_t>& address) {
  if (!ValidateAddress(address) || id >= rx_addresses_.size()) {
    return false;
  }

  rx_addresses_[id] = address;
  return true;
}

void NRF24::SetAutoAckEnabled(bool auto_ack_enabled) {
  auto_ack_enabled_ = auto_ack_enabled;
}

bool NRF24::SetCRCMode(CRCMode crc_mode) {
  switch (crc_mode) {
    case CRCMode::C8Bit:
    case CRCMode::C16Bit:
      crc_mode_ = CRCMode::C16Bit;
      return true;
  }

  return false;
}

bool NRF24::WriteConfig() {
  // Write the address width.
  uint8_t read_width;
  uint8_t width = (address_width_ >> 2);
  WriteRegister(kRegisterAddressWidth, width);
  ReadRegister(kRegisterAddressWidth, &read_width);
  if (read_width != width) {
    LOGE("Address width readback mismatch: got 0x%02x, expected 0x%02x",
        read_width, width);
    return false;
  }

  // Write the RF channel.
  uint8_t read_channel;
  WriteRegister(kRegisterChannel, channel_);
  ReadRegister(kRegisterChannel, &read_channel);
  if (read_channel != channel_) {
    LOGE("Channel readback mismatch: got 0x%02x, expected 0x%02x",
        read_channel, channel_);
    return false;
  }

  // Write the RF configuration.
  uint8_t rf_config = (static_cast<uint8_t>(power_level_) << 1)
      | ((static_cast<uint8_t>(data_rate_) & 0x01) << 3)
      | ((static_cast<uint8_t>(data_rate_) & 0x02) << 5);
  uint8_t read_rf_config;
  WriteRegister(kRegisterRFConfig, rf_config);
  ReadRegister(kRegisterRFConfig, &read_rf_config);
  if (read_rf_config != rf_config) {
    LOGE("RF config readback mismatch: got 0x%02x, expected 0x%02x",
        read_rf_config, rf_config);
    return false;
  }

  // Write the transmit address if specified.
  if (!tx_address_.empty()) {
    std::vector<uint8_t> read_address(tx_address_.size());
    WriteRegister(kRegisterTxAddress, tx_address_);
    ReadRegister(kRegisterTxAddress, read_address);
    if (read_address != tx_address_) {
      LOGE("Tx address readback mismatch");
      return false;
    }
  }

  // Write receive addresses if specified.
  uint8_t receive_pipe_enabled = 0x00;
  for (size_t i = 0; i < rx_addresses_.size(); i++) {
    const auto& rx_address = rx_addresses_[i];
    if (rx_address.empty()) {
      continue;
    }

    receive_pipe_enabled |= 1 << i;
    std::vector<uint8_t> read_address(rx_address.size());
    WriteRegister(kRegisterRxAddressBase + i, rx_address);
    ReadRegister(kRegisterRxAddressBase + i, read_address);
    if (read_address != rx_address) {
      LOGE("Rx address readback mismatch");
      return false;
    }
  }

  // Write receive pipe enables.
  uint8_t read_receive_pipe_enabled;
  WriteRegister(kRegisterReceiveAddress, receive_pipe_enabled);
  ReadRegister(kRegisterReceiveAddress, &read_receive_pipe_enabled);
  if (read_receive_pipe_enabled != receive_pipe_enabled) {
    LOGE("Receive pipe enabled mismatch: got 0x%02x, expected 0x%02x",
        read_receive_pipe_enabled, receive_pipe_enabled);
    return false;
  }

  // Write the auto-ack enabled.
  uint8_t auto_ack = auto_ack_enabled_ ? 0x3f : 0x00;
  uint8_t read_auto_ack;
  WriteRegister(kRegisterAutoAck, auto_ack);
  ReadRegister(kRegisterAutoAck, &read_auto_ack);
  if (read_auto_ack != auto_ack) {
    LOGE("Auto ack enabled mismatch: got 0x%02x, expected 0x%02x",
        read_auto_ack, auto_ack);
    return false;
  }

  return WriteConfigRegister();
}

bool NRF24::WriteConfigRegister() {
  uint8_t config = 0x7c | (static_cast<uint8_t>(crc_mode_) << 3)
      | in_receive_mode_;
  uint8_t read_config;
  WriteRegister(kRegisterConfig, config);
  ReadRegister(kRegisterConfig, &read_config);
  if (read_config != config) {
    LOGE("Config mismatch: got 0x%02x, expected 0x%02x",
        read_config, config);
    return false;
  }

  return true;
}

bool NRF24::ValidateAddress(const std::vector<uint8_t>& address, size_t id) {
  if (id < 2 && address.size() >= 3 && address.size() <= 5) {
    return true;
  }

  if (id < 5 && address.size() == 1) {
    return true;
  }

  return false;
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
