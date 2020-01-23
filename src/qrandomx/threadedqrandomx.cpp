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

#include "threadedqrandomx.h"
#include "qrandomx/qrandomx.h"

ThreadedQRandomX::ThreadedQRandomX() {
  _eventThread = std::make_unique<std::thread>([&]() { _threadedQRandomXProxy(); });
}

ThreadedQRandomX::~ThreadedQRandomX() {
  {
    std::lock_guard<std::mutex> queue_lock(_eventQueue_mutex);
    _stop_eventThread = true;
    _eventReleased.notify_one();
  }
  _eventThread->join();
}

void ThreadedQRandomX::_submitWork(std::shared_ptr<QRandomXParams>& qrxParams) {
  std::lock_guard<std::mutex> lock_queue(_eventQueue_mutex);
  _eventQueue.push_back(qrxParams);
  _eventReleased.notify_one();
}

void ThreadedQRandomX::freeVM() {

}

uint64_t ThreadedQRandomX::getSeedHeight(const uint64_t blockNumber) {
  std::shared_ptr<QRandomXParams> qrxParams = std::make_shared<QRandomXParams>(blockNumber);

  _submitWork(qrxParams);
  std::unique_lock<std::mutex> outputQLock(qrxParams->_outputQueue_mutex);
  qrxParams->_outputReady.wait(outputQLock, [=] { return !qrxParams->_output.empty() || _stop_eventThread; });
  return qrxParams->_output.front().heightOutput;
}

std::vector<uint8_t> ThreadedQRandomX::hash(const uint64_t mainHeight,
        const uint64_t seedHeight, const std::vector<uint8_t>& seedHash,
        const std::vector<uint8_t>& input, int miners) {

  std::shared_ptr<QRandomXParams> qrxParams = std::make_shared<QRandomXParams>(mainHeight,
          seedHeight, seedHash, input, miners);
  _submitWork(qrxParams);

  // Check outputReady
  std::unique_lock<std::mutex> outputQLock(qrxParams->_outputQueue_mutex);
  qrxParams->_outputReady.wait(outputQLock, [=] { return !qrxParams->_output.empty() || _stop_eventThread; });

  return qrxParams->_output.front().hashOutput;
}

void ThreadedQRandomX::_threadedQRandomXProxy() {
  std::unique_ptr<QRandomX> qrx(new QRandomX);
  while (!_stop_eventThread) {
    std::unique_lock<std::mutex> queue_lock(_eventQueue_mutex);
    _eventReleased.wait(queue_lock,
                        [=] { return !_eventQueue.empty() || _stop_eventThread; });
    if(!_eventQueue.empty()) {
      std::vector<std::shared_ptr<QRandomXParams>> events;
      while (!_eventQueue.empty()) {
        auto event = _eventQueue.front();
        events.push_back(event);
        _eventQueue.pop_front();
      }
      queue_lock.unlock();

      // Process Request
      for(const auto& event: events) {
        QRandomXProxyResult qrxResult;
        switch (event->funcType) {
          case 0:
            qrxResult.hashOutput = qrx->hash(event->mainHeight, event->seedHeight,
                                             event->seedHash, event->input, event->miners);
            break;
          case 1:
            qrxResult.heightOutput = qrx->getSeedHeight(event->mainHeight);
            break;
          case 2:
            qrx->freeVM();
            break;
        }
        // Notify output is ready
        std::lock_guard<std::mutex> lock_queue(event->_outputQueue_mutex);
        event->_output.push_back(qrxResult);
        event->_outputReady.notify_one();
      }
    } else {
      queue_lock.unlock();
    }
  }
}