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

#ifndef QRANDOMX_QRANDOMX_H
#define QRANDOMX_QRANDOMX_H

#include <pthread.h>
#include <string>
#include <vector>
#include <array>

class QRandomX {
public:
    virtual ~QRandomX();

    void freeVM();

    std::string lastError() { return std::string(""); };

    static uint64_t getSeedHeight(const uint64_t blockNumber);

    static std::vector<uint8_t> hash(const uint64_t mainHeight,
            const uint64_t seedHeight, const std::vector<uint8_t>& seedHash,
            const std::vector<uint8_t>& input, int miners);

};

#endif //QRANDOMX_QRANDOMX_H
