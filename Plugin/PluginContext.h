/**
 * Transfers accelerator plugin for Orthanc
 * Copyright (C) 2018 Osimis, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#pragma once

#include "../Framework/OrthancInstancesCache.h"
#include "../Framework/PushMode/ActivePushTransactions.h"

#include <Core/MultiThreading/Semaphore.h>

#include <map>

namespace OrthancPlugins
{
  class PluginContext : public boost::noncopyable
  {
  private:
    // Runtime structures
    OrthancPluginContext*    context_;
    OrthancInstancesCache    cache_;
    ActivePushTransactions   pushTransactions_;
    Orthanc::Semaphore       semaphore_;
    std::string              pluginUuid_;

    // Configuration
    size_t                   threadsCount_;
    size_t                   targetBucketSize_;

  
    PluginContext(OrthancPluginContext* context,
                  size_t threadsCount,
                  size_t targetBucketSize,
                  size_t maxPushTransactions,
                  size_t memoryCacheSize);

    static std::auto_ptr<PluginContext>& GetSingleton();
  
  public:
    OrthancPluginContext* GetOrthanc()
    {
      return context_;
    }
    
    OrthancInstancesCache& GetCache()
    {
      return cache_;
    }

    ActivePushTransactions& GetActivePushTransactions()
    {
      return pushTransactions_;
    }

    Orthanc::Semaphore& GetSemaphore()
    {
      return semaphore_;
    }

    const std::string& GetPluginUuid() const
    {
      return pluginUuid_;
    }

    size_t GetThreadsCount() const
    {
      return threadsCount_;
    }

    size_t GetTargetBucketSize() const
    {
      return targetBucketSize_;
    }

    static void Initialize(OrthancPluginContext* context,
                           size_t threadsCount,
                           size_t targetBucketSize,
                           size_t maxPushTransactions,
                           size_t memoryCacheSize);
  
    static PluginContext& GetInstance();

    static void Finalize();
  };
}
