/*
 * Copyright 2021 Andrew Rossignol andrew.rossignol@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NERFNET_UTIL_CRC16_H_
#define NERFNET_UTIL_CRC16_H_

#include <string>

namespace nerfnet {

// Returns a CRC16 of the provided buffer.
uint16_t GenerateCrc16(const std::string& buffer);

}  // namespace nerfnet

#endif  // NERFNET_UTIL_CRC16_H_
