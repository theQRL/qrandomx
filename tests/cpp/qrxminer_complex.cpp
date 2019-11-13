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
#include <iostream>
#include <xmmintrin.h>
#include <qrandomx/qrxminer.h>
#include <misc/bignum.h>
#include <pow/powhelper.h>
#include <qrandomx/threadedqrandomx.h>
#include "gtest/gtest.h"

#define MINEXPECTEDMXCSR 8064
#define MAXEXPECTEDMXCSR 8127

namespace {
  class CustomMiner: public QRXMiner
  {
  public:
    uint8_t handleEvent(MinerEvent event) override
    {
      if (event.type == SOLUTION)
      {
        using namespace std::chrono_literals;
        std::cout << "custom nonce: " << event.nonce << std::endl;
        std::this_thread::sleep_for(500ms);
        cancel();
      }
      return 1;
    }
  };

  TEST(QRXMiner, CancelInEvent) {
    CustomMiner qrxm;
    ThreadedQRandomX qrx;

    uint64_t main_height = 10;
    uint64_t seed_height = qrx.getSeedHeight(main_height);

    std::vector<uint8_t> seed_hash {
            0x2a, 0x1c, 0x4a, 0x94, 0x33, 0xf1, 0xde, 0x36,
            0xf8, 0xb9, 0x9c, 0x7c, 0x5a, 0xce, 0xb7, 0xbd,
            0x2e, 0xb3, 0x9e, 0x1e, 0xad, 0x64, 0x8e, 0xa5,
            0x82, 0x27, 0xd3, 0x99, 0xad, 0x84, 0xc7, 0x24
    };

    std::vector<uint8_t> input {
            0x00, 0x9d, 0x6b, 0x07, 0x38, 0x20, 0xb0, 0x0a,
            0x5b, 0x60, 0xc7, 0x64, 0x6f, 0x3d, 0x30, 0x46,
            0xa3, 0x49, 0x49, 0xcd, 0x93, 0x9b, 0x2b, 0x52,
            0xb9, 0x9f, 0x1b, 0xb2, 0xde, 0x1f, 0x4c, 0x92,
            0x12, 0x7a, 0xe9, 0xe4, 0xc3, 0xe3, 0x9e, 0xe0,
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x58,
            0x22, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x54, 0xdb, 0x8c, 0x8e, 0xa6, 0x6f, 0xc0, 0x5b,
            0x1d, 0x12, 0x24, 0x25, 0x5a, 0x24, 0x70, 0x8e,
            0x5d, 0x67, 0x33, 0xdc
    };

    std::vector<uint8_t> boundary = {
            0x9F, 0xFF, 0xFF, 0xE1, 0xAC, 0xF3, 0x55, 0x92,
            0x66, 0xD8, 0x43, 0x89, 0xCE, 0xDE, 0x99, 0x33,
            0xC6, 0x8F, 0xC5, 0x1E, 0xD0, 0xA6, 0xC7, 0x91,
            0xF8, 0xF9, 0xE8, 0x9D, 0xB6, 0x23, 0xF0, 0xFF
    };

    for(int i=0; i<20; i++)
    {
        using namespace std::chrono_literals;
        boundary[0]-=10;
        qrxm.start(main_height, seed_height, seed_hash, input, 0, boundary);
        std::this_thread::sleep_for(500ms);
        qrxm.cancel();
    }
    ASSERT_GE(_mm_getcsr(), MINEXPECTEDMXCSR);
    ASSERT_LE(_mm_getcsr(), MAXEXPECTEDMXCSR);
  }
}