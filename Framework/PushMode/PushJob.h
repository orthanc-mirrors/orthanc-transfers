/**
 * Transfers accelerator plugin for Orthanc
 * Copyright (C) 2018-2019 Osimis S.A., Belgium
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

#include "../OrthancInstancesCache.h"
#include "../StatefulOrthancJob.h"
#include "../TransferQuery.h"

namespace OrthancPlugins
{
  class PushJob : public StatefulOrthancJob
  {
  private:
    class CreateTransactionState;    
    class PushBucketsState;
    class FinalState;

    OrthancInstancesCache&   cache_;
    TransferQuery            query_;
    size_t                   threadsCount_;
    size_t                   targetBucketSize_;
    OrthancPeers             peers_;
    size_t                   peerIndex_;
    
    virtual StateUpdate* CreateInitialState(JobInfo& info);
    
  public:
    PushJob(const TransferQuery& query,
            OrthancInstancesCache& cache,
            size_t threadsCount,
            size_t targetBucketSize);
  };
}
