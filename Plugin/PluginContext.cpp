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


#include "PluginContext.h"

#include <Core/Logging.h>


namespace OrthancPlugins
{
  PluginContext::PluginContext(OrthancPluginContext* context,
                               size_t threadsCount,
                               size_t targetBucketSize,
                               size_t maxPushTransactions,
                               size_t memoryCacheSize) :
    context_(context),
    cache_(context),
    pushTransactions_(maxPushTransactions),
    semaphore_(threadsCount),
    threadsCount_(threadsCount),
    targetBucketSize_(targetBucketSize)
  {
    pluginUuid_ = Orthanc::Toolbox::GenerateUuid();

    cache_.SetMaxMemorySize(memoryCacheSize);

    LOG(INFO) << "Transfers accelerator will use " << threadsCount << " threads to run HTTP queries";
    LOG(INFO) << "Transfers accelerator will use keep local DICOM files in a memory cache of size: "
              << OrthancPlugins::ConvertToMegabytes(memoryCacheSize) << " MB";
    LOG(INFO) << "Transfers accelerator will aim at HTTP queries of size: "
              << OrthancPlugins::ConvertToKilobytes(targetBucketSize) << " KB";
    LOG(INFO) << "Transfers accelerator will be able to receive up to "
              << maxPushTransactions << " push transactions at once";

  }


  std::auto_ptr<PluginContext>& PluginContext::GetSingleton()
  {
    static std::auto_ptr<PluginContext>  singleton_;
    return singleton_;
  }

  
  bool PluginContext::LookupBidirectionalPeer(std::string& remoteSelf,
                                              const std::string& remotePeer) const
  {
    BidirectionalPeers::const_iterator found = bidirectionalPeers_.find(remotePeer);

    if (found == bidirectionalPeers_.end())
    {
      return false;
    }
    else
    {
      remoteSelf = found->second;
      return true;
    }
  }
  

  void PluginContext::Initialize(OrthancPluginContext* context,
                                 size_t threadsCount,
                                 size_t targetBucketSize,
                                 size_t maxPushTransactions,
                                 size_t memoryCacheSize)
  {
    GetSingleton().reset(new PluginContext(context, threadsCount, targetBucketSize,
                                           maxPushTransactions, memoryCacheSize));
  }

  
  PluginContext& PluginContext::GetInstance()
  {
    if (GetSingleton().get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return *GetSingleton();
    }
  }
  

  void PluginContext::Finalize()
  {
    if (GetSingleton().get() != NULL)
    {
      GetSingleton().reset();
    }
  }
}
