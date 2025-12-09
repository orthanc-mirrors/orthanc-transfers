/**
 * Transfers accelerator plugin for Orthanc
 * Copyright (C) 2018-2023 Osimis S.A., Belgium
 * Copyright (C) 2024-2025 Orthanc Team SRL, Belgium
 * Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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

#include "TransferScheduler.h"

#include <TemporaryFile.h>
#include <boost/thread/thread.hpp>
#include <MultiThreading/SharedMessageQueue.h>

namespace OrthancPlugins
{
  class DownloadArea : public boost::noncopyable
  {
  private:
    class InstanceToCommit;

    class Instance : public boost::noncopyable
    {
    private:
      DicomInstanceInfo       info_;
      Orthanc::TemporaryFile  file_;

      class Writer;

    public:
      explicit Instance(const DicomInstanceInfo& info);
      
      virtual ~Instance()
      {}

      const DicomInstanceInfo& GetInfo() const
      {
        return info_;
      }

      void WriteChunk(size_t offset,
                      const void* data,
                      size_t size);

      void Commit(bool simulate) const;
    };


    typedef std::map<std::string, Instance*>   Instances;

    boost::mutex  instancesMutex_;
    Instances     instances_;
    size_t        totalSize_;
    std::vector<boost::shared_ptr<boost::thread> > commitThreads_;
    Orthanc::SharedMessageQueue instancesToCommit_;
    bool          workersShouldStop_;

    void Clear();

    void ClearThreads();

    Instance& LookupInstance(const std::string& id);

    void WriteUncompressedBucket(const TransferBucket& bucket,
                                 const void* data,
                                 size_t size);

    void Setup(const std::vector<DicomInstanceInfo>& instances);
    
    void CommitInternal(bool simulate);

    static void CommitWorker(DownloadArea* that);
  public:
    explicit DownloadArea(const TransferScheduler& scheduler);

    explicit DownloadArea(const std::vector<DicomInstanceInfo>& instances);

    ~DownloadArea()
    {
      Clear();
    }

    size_t GetTotalSize() const
    {
      return totalSize_;
    }

    void WriteBucket(const TransferBucket& bucket,
                     const void* data,
                     size_t size,
                     BucketCompression compression);

    void WriteInstance(const std::string& instanceId,
                       const void* data,
                       size_t size);

    void CheckMD5();

    void Commit();

    static void SetCommitWorkerThreadsCount(uint32_t workersCount);
  };
}
