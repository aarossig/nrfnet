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

#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <RF24/RF24.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <tclap/CmdLine.h>
#include <unistd.h>

#include "nerfnet/net/primary_radio_interface.h"
#include "nerfnet/net/secondary_radio_interface.h"
#include "nerfnet/util/log.h"

// A description of the program.
constexpr char kDescription[] =
    "A tool for creating a network tunnel over cheap NRF24L01 radios.";

// The version of the program.
constexpr char kVersion[] = "0.0.1";

// Sets flags for a given interface. Quits and logs the error on failure.
void SetInterfaceFlags(const std::string_view& device_name, int flags) {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  CHECK(fd >= 0, "Failed to open socket: %s (%d)", strerror(errno), errno);

  struct ifreq ifr = {};
  ifr.ifr_flags = flags;
  strncpy(ifr.ifr_name, std::string(device_name).c_str(), IFNAMSIZ);
  int status = ioctl(fd, SIOCSIFFLAGS, &ifr);
  CHECK(status >= 0, "Failed to set tunnel interface: %s (%d)",
      strerror(errno), errno);
  close(fd);
}

// Opens the tunnel interface to listen on. Always returns a valid file
// descriptor or quits and logs the error.
int OpenTunnel(const std::string_view& device_name) {
  int fd = open("/dev/net/tun", O_RDWR);
  CHECK(fd >= 0, "Failed to open tunnel file: %s (%d)", strerror(errno), errno);

  struct ifreq ifr = {};
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  strncpy(ifr.ifr_name, std::string(device_name).c_str(), IFNAMSIZ);

  int status = ioctl(fd, TUNSETIFF, &ifr);
  CHECK(status >= 0, "Failed to set tunnel interface: %s (%d)",
      strerror(errno), errno);
  return fd;
}

int main(int argc, char** argv) {
  // Parse command-line arguments.
  TCLAP::CmdLine cmd(kDescription, ' ', kVersion);
  TCLAP::ValueArg<std::string> interface_name_arg("i", "interface_name",
      "Set to the name of the tunnel device.", false, "nerf0", "name", cmd);
  TCLAP::ValueArg<uint16_t> ce_pin_arg("", "ce_pin",
      "Set to the index of the NRF24L01 chip-enable pin.", false, 22, "index",
      cmd);
  TCLAP::SwitchArg primary_arg("", "primary",
      "Run this side of the network in primary mode.", false);
  TCLAP::SwitchArg secondary_arg("", "secondary",
      "Run this side of the network in secondary mode.", false);
  cmd.xorAdd(primary_arg, secondary_arg);
  TCLAP::ValueArg<uint32_t> primary_addr_arg("", "primary_addr",
      "The address to use for the primary side of nerfnet.",
      false, 0x90019001, "address", cmd);
  TCLAP::ValueArg<uint32_t> secondary_addr_arg("", "secondary_addr",
      "The address to use for the secondary side of nerfnet.",
      false, 0x90009000, "address", cmd);
  TCLAP::ValueArg<uint32_t> rf_delay_us_arg("", "rf_delay_us",
      "The delay between RF operations in microseconds.",
      false, 5000, "microseconds", cmd);
  TCLAP::SwitchArg ping_arg("", "ping",
      "Only used by the primary radio. Issue a ping request then quit.", cmd);
  cmd.parse(argc, argv);

  // Setup tunnel.
  int tunnel_fd = OpenTunnel(interface_name_arg.getValue());
  LOGI("tunnel '%s' opened", interface_name_arg.getValue().c_str());
  SetInterfaceFlags(interface_name_arg.getValue(), IFF_UP);
  LOGI("tunnel '%s' up", interface_name_arg.getValue().c_str());

  if (primary_arg.getValue()) {
    nerfnet::PrimaryRadioInterface radio_interface(
        ce_pin_arg.getValue(), tunnel_fd,
        primary_addr_arg.getValue(), secondary_addr_arg.getValue(),
        rf_delay_us_arg.getValue());
    if (ping_arg.getValue()) {
      auto result = radio_interface.Ping(1337);
      if (result == nerfnet::RadioInterface::RequestResult::Success) {
        LOGI("ping completed successfully");
      }
    } else {
      radio_interface.Run();
    }
  } else if (secondary_arg.getValue()) {
    nerfnet::SecondaryRadioInterface radio_interface(
        ce_pin_arg.getValue(), tunnel_fd,
        primary_addr_arg.getValue(), secondary_addr_arg.getValue(),
        rf_delay_us_arg.getValue());
    radio_interface.Run();
  } else {
    CHECK(false, "Primary or secondary mode must be enabled");
  }

  return 0;
}
