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

#include <array>
#include <string>
#include <vector>

#include "nerfnet/util/non_copyable.h"

namespace nerfnet {

// A spidev-based driver for the NRF24L01 chipset.
class NRF24 : public NonCopyable {
 public:
  // Setup the NRF24 device.
  NRF24(const std::string& spidev_path, uint16_t ce_pin);

  // Cleanup resources associated with this device.
  ~NRF24();

  // Sets the channel to transmit/receive on. Returns false if the channnel
  // is invalid.
  bool SetChannel(uint8_t channel);

  // Power levels supported by this radio.
  enum class PowerLevel {
    Min = 0x00,   // -18dBm
    Med = 0x01,   // -12dBm
    High = 0x02,  // -6dBm
    Max = 0x03,   // 0dBm
  };

  // Sets the power level to transmit at. Returns false if the power level
  // is invalid.
  bool SetPowerLevel(PowerLevel power_level);

  enum class DataRate {
    R1MBPS = 0x00,
    R2MBPS = 0x01,
    R250KBPS = 0x10,
  };

  // Sets the data rate to transmit at. Returns false if the data rate
  // is invalid.
  bool SetDataRate(DataRate data_rate);

  // Handle setting the address width used on the air. This will truncate
  // addresses if they are longer than the value specified here.
  bool SetAddressWidth(size_t address_width);

  // Handle setting addresses for different pipes. Returns false if the
  // arguments are invalid (address too short, invalid pipe id).
  bool SetTransmitAddress(const std::vector<uint8_t>& address);
  bool SetReceiveAddress(size_t id, const std::vector<uint8_t>& address);

  // Sets the automatic ankowledgement feature enabled.
  void SetAutoAckEnabled(bool auto_ack_enabled);

  // The possible CRC modes.
  enum class CRCMode {
    C8Bit = 0x00,
    C16Bit = 0x01,
  };

  // Sets the CRC mode.
  bool SetCRCMode(CRCMode mode);

  // Sets the retransmit parameters. The first is a multiplier for how many
  // microseconds to wait between retransmits. The formula is:
  //
  // (wait_250us + 1) * 250us.
  //
  // The second param is the maximum number of retransmits.
  //
  // Returns false if any of these params are out of range.
  bool SetRetransmitParams(uint8_t wait_250us, uint8_t count);

  // Writes the configuration to the radio. This is called upon creation of an
  // instance of this driver. It should be called to commit any other changes
  // such as setting the channel or data rate. False is returned on error and
  // errors are logged.
  bool WriteConfig();

  // Enables receive mode. Returns false on error and logs the error.
  bool EnterReceiveMode();

 private:
  // The GPIO for chip-enable.
  const uint16_t ce_pin_;

  // The channel to transmit/receive on.
  uint8_t channel_;

  // The power level and data rate to transmit at.
  PowerLevel power_level_;
  DataRate data_rate_;

  // Address configurations for the various pipes.
  size_t address_width_;
  std::vector<uint8_t> tx_address_;
  std::array<std::vector<uint8_t>, 6> rx_addresses_;

  // Set to true when auto acknowlegdement is enabled.
  bool auto_ack_enabled_;

  // The CRC mode for on-air packets.
  CRCMode crc_mode_;

  // Set to true when the chip is in receive mode.
  bool in_receive_mode_;

  // The parameters for retransmits.
  uint8_t retransmit_wait_250us_;
  uint8_t retransmit_count_;

  // The file descriptor for the spidev device used to perform SPI
  // transactions.
  int spi_fd_;

  // Writes the config register.
  bool WriteConfigRegister();

  // Validates the length of a given address.
  bool ValidateAddress(const std::vector<uint8_t>& address, size_t id = 0);

  // Writes the supplied value to a given register.
  void WriteRegister(uint8_t address, uint8_t value);
  void WriteRegister(uint8_t address, const std::vector<uint8_t>& value);

  // Reads the value from a given register.
  void ReadRegister(uint8_t address, uint8_t* value);
  void ReadRegister(uint8_t address, std::vector<uint8_t>& value);

  // Setup the SPI device for the supplied path. All errors are fatal and
  // logged.
  void SetupSPIDevice(const std::string& spidev_path);

  // Performs a SPI transaction with the supplied buffers.
  void PerformSPITransaction(const std::vector<uint8_t>& tx_buffer);
  void PerformSPITransaction(const std::vector<uint8_t>& tx_buffer,
                             std::vector<uint8_t>& rx_buffer);

  // Initializes the chip-enable line.
  void InitChipEnable();

  // Set the value of the chip-enable line.
  void SetChipEnable(bool value);
};

}  // namespace nerfnet

#endif  // NERFNET_DRIVER_NRF24_H_
