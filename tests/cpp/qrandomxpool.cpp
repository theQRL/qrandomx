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
#include <qrandomx/qrandomxpool.h>
#include "gtest/gtest.h"

#define MINEXPECTEDMXCSR 8064
#define MAXEXPECTEDMXCSR 8127

namespace {
  class QRandomXWithRefCount : public ThreadedQRandomX
  {
  public:
      QRandomXWithRefCount() : ThreadedQRandomX() { ++_instances; }
      virtual ~QRandomXWithRefCount() { --_instances; }
      static size_t _instances;
  };

  size_t QRandomXWithRefCount::_instances = 0;

  QRandomXPool::QRandomXFactory factory =
          [](){ return new QRandomXWithRefCount(); };

  void ValidateHash(QRandomXPool::uniqueQRandomXPtr& qrx)
  {
    uint64_t main_height = 10;
    uint64_t seed_height = qrx->getSeedHeight(main_height);

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

    int miners = 0;

    std::vector<uint8_t> output_expected {
            0x36, 0x15, 0x33, 0x94, 0xa3, 0xd9, 0x01, 0x0d,
            0xef, 0xae, 0x86, 0x31, 0xb9, 0xa6, 0xbb, 0x1f,
            0x9d, 0x62, 0x9d, 0x54, 0x8b, 0xc7, 0x3f, 0xbe,
            0x61, 0x2c, 0xfe, 0x6b, 0x08, 0xd7, 0xa2, 0x16
    };

    auto output = qrx->hash(main_height, seed_height, seed_hash, input, miners);

    EXPECT_EQ(output_expected, output);
    ASSERT_GE(_mm_getcsr(), MINEXPECTEDMXCSR);
    ASSERT_LE(_mm_getcsr(), MAXEXPECTEDMXCSR);
  }

  TEST(QRandomXPool, Init) {
    auto pool = std::make_shared<QRandomXPool>(factory);
    EXPECT_TRUE(pool->empty());
    EXPECT_EQ(pool->size(), 0);
    EXPECT_EQ(QRandomXWithRefCount::_instances, 0);
    ASSERT_GE(_mm_getcsr(), MINEXPECTEDMXCSR);
    ASSERT_LE(_mm_getcsr(), MAXEXPECTEDMXCSR);
  }

  TEST(QRandomXPool, Empty) {
    auto pool = std::make_shared<QRandomXPool>(factory);

    auto qrx = pool->acquire();
    EXPECT_TRUE(pool->empty());
    EXPECT_EQ(QRandomXWithRefCount::_instances, 1);

    qrx.reset();
    EXPECT_FALSE(pool->empty());
    EXPECT_EQ(QRandomXWithRefCount::_instances, 1);
    ASSERT_GE(_mm_getcsr(), MINEXPECTEDMXCSR);
    ASSERT_LE(_mm_getcsr(), MAXEXPECTEDMXCSR);
  }

  TEST(QRandomXPool, AcquireHashReleaseCycle) {
    auto pool = std::make_shared<QRandomXPool>(factory);

    auto qrx = pool->acquire();
    EXPECT_EQ(pool->size(), 0);
    EXPECT_EQ(QRandomXWithRefCount::_instances, 1);

    ValidateHash(qrx);

    qrx.reset();
    EXPECT_EQ(pool->size(), 1);
    EXPECT_EQ(QRandomXWithRefCount::_instances, 1);

    qrx = pool->acquire();
    EXPECT_EQ(pool->size(), 0);
    EXPECT_EQ(QRandomXWithRefCount::_instances, 1);

    ValidateHash(qrx);

    qrx.reset();
    EXPECT_EQ(pool->size(), 1);
    EXPECT_EQ(QRandomXWithRefCount::_instances, 1);

    pool.reset();
    EXPECT_EQ(QRandomXWithRefCount::_instances, 0);
    ASSERT_GE(_mm_getcsr(), MINEXPECTEDMXCSR);
    ASSERT_LE(_mm_getcsr(), MAXEXPECTEDMXCSR);
  }

  TEST(QRandomXPool, AcquireHashReleaseFour) {
    auto pool = std::make_shared<QRandomXPool>(factory);

    auto qrx1 = pool->acquire();
    auto qrx2 = pool->acquire();
    auto qrx3 = pool->acquire();
    auto qrx4 = pool->acquire();
    EXPECT_EQ(pool->size(), 0);
    EXPECT_EQ(QRandomXWithRefCount::_instances, 4);

    ValidateHash(qrx1);
    ValidateHash(qrx2);
    ValidateHash(qrx3);
    ValidateHash(qrx4);

    qrx1.reset();
    qrx2.reset();
    qrx3.reset();
    qrx4.reset();
    EXPECT_EQ(pool->size(), 4);
    EXPECT_EQ(QRandomXWithRefCount::_instances, 4);

    pool.reset();
    EXPECT_EQ(QRandomXWithRefCount::_instances, 0);
    ASSERT_GE(_mm_getcsr(), MINEXPECTEDMXCSR);
    ASSERT_LE(_mm_getcsr(), MAXEXPECTEDMXCSR);
  }

  TEST(QRandomXPool, AcquireAndDeletePool) {
    auto pool = std::make_shared<QRandomXPool>(factory);

    auto qrx = pool->acquire();
    EXPECT_EQ(pool->size(), 0);
    EXPECT_EQ(QRandomXWithRefCount::_instances, 1);

    pool.reset();

    EXPECT_EQ(QRandomXWithRefCount::_instances, 1);

    ValidateHash(qrx);

    qrx.reset();

    EXPECT_EQ(QRandomXWithRefCount::_instances, 0);
    ASSERT_GE(_mm_getcsr(), MINEXPECTEDMXCSR);
    ASSERT_LE(_mm_getcsr(), MAXEXPECTEDMXCSR);
  }

}