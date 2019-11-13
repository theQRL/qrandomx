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

#include "qrandomxpool.h"

QRandomXPool::ReturnToPoolDeleter::ReturnToPoolDeleter(std::weak_ptr<QRandomXPool> ptrToOwnerPool)
        : _ptrToOwnerPool(ptrToOwnerPool) { }

void QRandomXPool::ReturnToPoolDeleter::operator()(ThreadedQRandomX* ptrToReleasedObject)
{
  if (auto pool = _ptrToOwnerPool.lock())
  {
    // the pool still exists so attempt to return it back to the pool
    try
    {
      pool->add(uniqueQRandomXPtr{ptrToReleasedObject, ReturnToPoolDeleter(pool)});
      return;
    }
    catch(const std::bad_alloc&) { }
  }
  // the pool is gone or cannot be added back so delete the object
  delete ptrToReleasedObject;
}

void QRandomXPool::ReturnToPoolDeleter::detachFromPool()
{
  _ptrToOwnerPool.reset();
}

QRandomXPool::QRandomXPool(QRandomXFactory factory)
        : _factory(factory)
        , _mutex()
        , _poolContainer()
{
}

QRandomXPool::~QRandomXPool()
{
  std::unique_lock<std::mutex> lock(_mutex);
  while (!_poolContainer.empty())
  {
    _poolContainer.top().get_deleter().detachFromPool();
    _poolContainer.pop();
  }
}

QRandomXPool::uniqueQRandomXPtr QRandomXPool::acquire()
{
  std::unique_lock<std::mutex> lock(_mutex);
  if (_poolContainer.empty())
  {
    // no QRandomX instances available in the pool so use the factory to
    // create and return a new one
    return uniqueQRandomXPtr{_factory(), ReturnToPoolDeleter(shared_from_this())};
  }
  else
  {
    // grab an unused QRandomX instance from the pool
    auto ptr = std::move(_poolContainer.top());
    _poolContainer.pop();
    return ptr;
  }
}

void QRandomXPool::add(QRandomXPool::uniqueQRandomXPtr ptr)
{
  std::unique_lock<std::mutex> lock(_mutex);
  _poolContainer.push(std::move(ptr));
}

bool QRandomXPool::empty() const
{
  std::unique_lock<std::mutex> lock(_mutex);
  return _poolContainer.empty();
}

size_t QRandomXPool::size() const
{
  std::unique_lock<std::mutex> lock(_mutex);
  return _poolContainer.size();
}
