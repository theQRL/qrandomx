/*
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  * Additional permission under GNU GPL version 3 section 7
  *
  * If you modify this Program, or any covered work, by linking or combining
  * it with OpenSSL (or a modified version of that library), containing parts
  * covered by the terms of OpenSSL License and SSLeay License, the licensors
  * of this Program grant you additional permission to convey the resulting work.
  *
  */

#include "qrandomx.h"
#include "rx-slow-hash.h"

QRandomX::~QRandomX() {
  freeVM();
}

void QRandomX::freeVM() {
  rx_slow_hash_free_state();
}

uint64_t QRandomX::getSeedHeight(const uint64_t blockNumber) {
  return rx_seedheight(blockNumber);
}

std::vector<uint8_t> QRandomX::hash(const uint64_t mainHeight,
        const uint64_t seedHeight, const std::vector<uint8_t>& seedHash,
        const std::vector<uint8_t>& input, int miners) {

  std::vector<uint8_t> output(32);

  rx_slow_hash(mainHeight, seedHeight, (char *) seedHash.data(),
          input.data(), input.size(),
          (char *) output.data(), miners, 0);

  return output;
}

