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

#ifndef QRANDOMX_THREADEDQRANDOMX_H
#define QRANDOMX_THREADEDQRANDOMX_H

#include <atomic>
#include <thread>
#include <mutex>
#include <future>
#include <deque>
#include <vector>

struct QRandomXProxyResult {
  std::vector<uint8_t> hashOutput;
  uint64_t heightOutput;
};

class QRandomXParams {
public:
  QRandomXParams(uint64_t mainHeight) {
    this->mainHeight = mainHeight;
    this->funcType = 1;
  }

  QRandomXParams(uint64_t mainHeight,
                 uint64_t seedHeight,
                 const std::vector<uint8_t>& seedHash,
                 const std::vector<uint8_t>& input,
                 uint32_t miners) {
    this->mainHeight = mainHeight;
    this->seedHeight = seedHeight;
    this->seedHash = seedHash;
    this->input = input;
    this->miners = miners;
    this->funcType = 0;
  }

  uint64_t mainHeight;
  uint64_t seedHeight;
  std::vector<uint8_t> seedHash;
  std::vector<uint8_t> input;
  uint32_t miners;

protected:
  std::deque<QRandomXProxyResult> _output;
  std::mutex _outputQueue_mutex;
  std::condition_variable _outputReady;

  int funcType;

  friend class ThreadedQRandomX;
};

class ThreadedQRandomX {
public:
  ThreadedQRandomX();
  virtual ~ThreadedQRandomX();

  void _submitWork(std::shared_ptr<QRandomXParams>& qrxParams);
  void _threadedQRandomXProxy();

  void freeVM();

  std::string lastError() { return std::string(""); };

  uint64_t getSeedHeight(const uint64_t blockNumber);

  std::vector<uint8_t> hash(const uint64_t mainHeight,
                            const uint64_t seedHeight, const std::vector<uint8_t>& seedHash,
                            const std::vector<uint8_t>& input, int miners);

protected:
  std::atomic_bool _stop_eventThread{false};
  std::unique_ptr<std::thread> _eventThread;

  std::deque<std::shared_ptr<QRandomXParams>> _eventQueue;
  std::mutex _eventQueue_mutex;
  std::condition_variable _eventReleased;
};

#endif //QRANDOMX_THREADEDQRANDOMX_H
