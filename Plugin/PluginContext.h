/**
 * Transfers accelerator plugin for Orthanc
 * Copyright (C) 2018-2023 Osimis S.A., Belgium
 * Copyright (C) 2024-2024 Orthanc Team SRL, Belgium
 * Copyright (C) 2021-2024 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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

#include <Compatibility.h>  // For std::unique_ptr
#include <MultiThreading/Semaphore.h>

#include <map>

namespace OrthancPlugins
{
  class PluginContext : public boost::noncopyable
  {
  private:
    // Runtime structures
    OrthancInstancesCache    cache_;
    ActivePushTransactions   pushTransactions_;
    Orthanc::Semaphore       semaphore_;
    std::string              pluginUuid_;

    // Configuration
    size_t                   threadsCount_;
    size_t                   targetBucketSize_;
    unsigned int             maxHttpRetries_;
    unsigned int             peerConnectivityTimeout_;
  
    PluginContext(size_t threadsCount,
                  size_t targetBucketSize,
                  size_t maxPushTransactions,
                  size_t memoryCacheSize,
                  unsigned int maxHttpRetries,
                  unsigned int peerConnectivityTimeout);

    static std::unique_ptr<PluginContext>& GetSingleton();
  
  public:
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

    unsigned int GetMaxHttpRetries() const
    {
      return maxHttpRetries_;
    }

    unsigned int GetPeerConnectivityTimeout() const
    {
      return peerConnectivityTimeout_;
    }

    static void Initialize(size_t threadsCount,
                           size_t targetBucketSize,
                           size_t maxPushTransactions,
                           size_t memoryCacheSize,
                           unsigned int maxHttpRetries,
                           unsigned int peerConnectivityTimeout);
  
    static PluginContext& GetInstance();

    static void Finalize();
  };
}
