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


#include "DownloadArea.h"

#include "../Resources/Orthanc/Plugins/OrthancPluginCppWrapper.h"

#include <Compression/GzipCompressor.h>
#include <Logging.h>
#include <SystemToolbox.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace OrthancPlugins
{
  static uint32_t commitWorkerThreadsCount = 1;
  static boost::mutex commitThreadsCounterMutex;
  static uint32_t commitThreadsCounter = 0;

  void DownloadArea::SetCommitWorkerThreadsCount(uint32_t workersCount)
  {
    commitWorkerThreadsCount = workersCount;
  }

  class DownloadArea::InstanceToCommit : public Orthanc::IDynamicObject
  {
    DownloadArea::Instance* instance_;
    bool simulate_;
  
  public:
    InstanceToCommit(DownloadArea::Instance* instance /* transfer ownership */, bool simulate) :
      instance_(instance),
      simulate_(simulate)
    {}
    
    virtual ~InstanceToCommit()
    {
      delete instance_;
    }

    DownloadArea::Instance* GetInstance()
    {
      return instance_;
    }

    bool IsSimulate()
    {
      return simulate_;
    }
  };

  class DownloadArea::Instance::Writer : public boost::noncopyable
  {
  private:
    boost::filesystem::ofstream stream_;
        
  public:
    Writer(Orthanc::TemporaryFile& f,
           bool create) 
    {
      if (create)
      {
        // Create the file.
        stream_.open(f.GetPath(), std::ofstream::out | std::ofstream::binary);
      }
      else
      {
        // Open the existing file to modify it. The "in" mode is
        // necessary, otherwise previous content is lost by
        // truncation (as an ofstream defaults to std::ios::trunc,
        // the flag to truncate the existing content).
        stream_.open(f.GetPath(), std::ofstream::in | std::ofstream::out | std::ofstream::binary);
      }

      if (!stream_.good())
      {
        std::string path;

#if ORTHANC_FRAMEWORK_VERSION_IS_ABOVE(1, 12, 10)
        path = Orthanc::SystemToolbox::PathToUtf8(f.GetPath());
#else
        path = f.GetPath();
#endif

        throw Orthanc::OrthancException(Orthanc::ErrorCode_CannotWriteFile, std::string("Unable to write to ") + path);
      }
    }

    void Write(size_t offset,
               const void* data,
               size_t size)
    {
      stream_.seekp(offset);
      stream_.write(reinterpret_cast<const char*>(data), size);
    }
  };


  DownloadArea::Instance::Instance(const DicomInstanceInfo& info) :
    info_(info)
  {
    Writer writer(file_, true);

    // Create a sparse file of expected size
    if (info_.GetSize() != 0)
    {
      writer.Write(info_.GetSize() - 1, "", 1);
    }
  }


  void DownloadArea::Instance::WriteChunk(size_t offset,
                                          const void* data,
                                          size_t size)
  {
    if (offset + size > info_.GetSize())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange, "WriteChunk out of bounds");
    }
    else if (size > 0)
    {
      Writer writer(file_, false);
      writer.Write(offset, data, size);
    }
  }

  
  void DownloadArea::Instance::Commit(bool simulate) const
  {
    std::string content;
    Orthanc::SystemToolbox::ReadFile(content, file_.GetPath());

    std::string md5;
    Orthanc::Toolbox::ComputeMD5(md5, content);

    if (md5 == info_.GetMD5())
    {
      if (!simulate)
      {
        Json::Value result;
        if (!RestApiPost(result, "/instances", 
                         content.empty() ? NULL : content.c_str(), content.size(),
                         false))
        {
          LOG(ERROR) << "Cannot import a transfered DICOM instance into Orthanc: "
                     << info_.GetId();
          throw Orthanc::OrthancException(Orthanc::ErrorCode_CorruptedFile);
        }
      }
    }
    else
    {
      LOG(ERROR) << "Bad MD5 sum in a transfered DICOM instance: " << info_.GetId();
      throw Orthanc::OrthancException(Orthanc::ErrorCode_CorruptedFile);
    }
  }


  void DownloadArea::Clear()
  {
    boost::mutex::scoped_lock lock(instancesMutex_);

    for (Instances::iterator it = instances_.begin(); 
         it != instances_.end(); ++it)
    {
      if (it->second != NULL)
      {
        delete it->second;
        it->second = NULL;
      }
    }

    instances_.clear();
  }


  DownloadArea::Instance& DownloadArea::LookupInstance(const std::string& id)
  {
    boost::mutex::scoped_lock lock(instancesMutex_);

    Instances::iterator it = instances_.find(id);

    if (it == instances_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource, "Unknown instance");
    }
    else if (it->first != id ||
             it->second == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else
    {
      return *it->second;
    }
  }


  void DownloadArea::WriteUncompressedBucket(const TransferBucket& bucket,
                                             const void* data,
                                             size_t size)
  {
    if (size != bucket.GetTotalSize())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol, 
        "WriteUncompressedBucket: " + boost::lexical_cast<std::string>(size) + " != " + boost::lexical_cast<std::string>(bucket.GetTotalSize()));
    }

    if (size == 0)
    {
      return;
    }

    size_t pos = 0;

    for (size_t i = 0; i < bucket.GetChunksCount(); i++)
    {
      size_t chunkSize = bucket.GetChunkSize(i);
      size_t offset = bucket.GetChunkOffset(i);

      if (pos + chunkSize > size)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      Instance& instance = LookupInstance(bucket.GetChunkInstanceId(i));
      instance.WriteChunk(offset, reinterpret_cast<const char*>(data) + pos, chunkSize);

      pos += chunkSize;
    }

    if (pos != size)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  void DownloadArea::Setup(const std::vector<DicomInstanceInfo>& instances)
  {
    boost::mutex::scoped_lock lock(instancesMutex_);

    totalSize_ = 0;
    
    for (size_t i = 0; i < instances.size(); i++)
    {
      const std::string& id = instances[i].GetId();
        
      assert(instances_.find(id) == instances_.end());
      instances_[id] = new Instance(instances[i]);

      totalSize_ += instances[i].GetSize();
    }
  }

  
  void DownloadArea::CommitWorker(DownloadArea* that)
  {
    {
      boost::mutex::scoped_lock lock(commitThreadsCounterMutex);
      Orthanc::Logging::SetCurrentThreadName(std::string("TF-COMMIT-") + boost::lexical_cast<std::string>(commitThreadsCounter++));
      commitThreadsCounter %= 1000000;
    }

    while (true)
    {
      std::unique_ptr<DownloadArea::InstanceToCommit> instanceToCommit(dynamic_cast<DownloadArea::InstanceToCommit*>(that->instancesToCommit_.Dequeue(0)));
      if (instanceToCommit.get() == NULL || that->workersShouldStop_)  // that's the signal to exit the thread
      {
        LOG(INFO) << "Commit thread has completed";
        return;
      }

      instanceToCommit->GetInstance()->Commit(instanceToCommit->IsSimulate());
    }

  }

  void DownloadArea::CommitInternal(bool simulate)
  {
    commitThreads_.reserve(commitWorkerThreadsCount);

    for (uint32_t i = 0; i < commitWorkerThreadsCount; ++i)
    {
      commitThreads_.push_back(boost::shared_ptr<boost::thread>(new boost::thread(CommitWorker, this)));
    }

    {
      boost::mutex::scoped_lock lock(instancesMutex_);
      
      for (Instances::iterator it = instances_.begin(); 
          it != instances_.end(); ++it)
      {
        if (it->second != NULL)
        {
          instancesToCommit_.Enqueue(new DownloadArea::InstanceToCommit(it->second, simulate)); // transfers the ownership of the Instance to the queue
          it->second = NULL;
        }
        else
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
      }
    }

    ClearThreads();
  }

  void DownloadArea::ClearThreads()
  {
    for (uint32_t i = 0; i < commitWorkerThreadsCount; ++i)
    {
      instancesToCommit_.Enqueue(NULL); // exit message
    }

    instancesToCommit_.WaitEmpty(0);

    for (uint32_t i = 0; i < commitWorkerThreadsCount; ++i)
    {
      if (commitThreads_[i]->joinable())
      {
        commitThreads_[i]->join();
      }
    }

  }

  DownloadArea::DownloadArea(const std::vector<DicomInstanceInfo>& instances)
  : instancesToCommit_(0),
    workersShouldStop_(false)
  {
    Setup(instances);
  }


  DownloadArea::DownloadArea(const TransferScheduler& scheduler)
  : instancesToCommit_(0),
    workersShouldStop_(false)
  {
    std::vector<DicomInstanceInfo> instances;
    scheduler.ListInstances(instances);
    Setup(instances);
  }


  void DownloadArea::WriteBucket(const TransferBucket& bucket,
                                 const void* data,
                                 size_t size,
                                 BucketCompression compression)
  {
    switch (compression)
    {
      case BucketCompression_None:
        WriteUncompressedBucket(bucket, data, size);
        break;
          
      case BucketCompression_Gzip:
      {
        std::string uncompressed;
        Orthanc::GzipCompressor compressor;
        compressor.Uncompress(uncompressed, data, size);
        WriteUncompressedBucket(bucket, uncompressed.c_str(), uncompressed.size());
        break;
      }

      default:          
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  void DownloadArea::WriteInstance(const std::string& instanceId,
                                   const void* data,
                                   size_t size)
  {
    std::string md5;
    Orthanc::Toolbox::ComputeMD5(md5, data, size);
      
    {
      boost::mutex::scoped_lock lock(instancesMutex_);

      Instances::const_iterator it = instances_.find(instanceId);
      if (it == instances_.end() ||
          it->second == NULL ||
          it->second->GetInfo().GetId() != instanceId ||
          it->second->GetInfo().GetSize() != size ||
          it->second->GetInfo().GetMD5() != md5)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_CorruptedFile);
      }
      else
      {
        it->second->WriteChunk(0, data, size);
      }
    }
  }


  void DownloadArea::CheckMD5()
  {
    LOG(INFO) << "Checking MD5 sum without committing (testing)";
    CommitInternal(true);
  }


  void DownloadArea::Commit()
  {
    LOG(INFO) << "Importing transfered DICOM files from the temporary download area into Orthanc";
    CommitInternal(false);
  }
}
