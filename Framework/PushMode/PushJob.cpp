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


#include "PushJob.h"

#include "BucketPushQuery.h"
#include "../HttpQueries/HttpQueriesRunner.h"
#include "../TransferScheduler.h"

#include <Compatibility.h>  // For std::unique_ptr
#include <Logging.h>


namespace OrthancPlugins
{
  class PushJob::FinalState : public IState
  {
  private:
    const PushJob&  job_;
    JobInfo&        info_;
    std::string     transactionUri_;
    bool            isCommit_;
      
  public:
    FinalState(const PushJob& job,
               JobInfo& info,
               const std::string& transactionUri,
               bool isCommit) :
      job_(job),
      info_(info),
      transactionUri_(transactionUri),
      isCommit_(isCommit)
    {
    }

    virtual StateUpdate* Step()
    {
      Json::Value answer;
      bool success = false;
      std::map<std::string, std::string> headers;
      job_.query_.GetHttpHeaders(headers);

      if (isCommit_)
      {
        success = DoPostPeer(answer, job_.peers_, job_.peerIndex_, transactionUri_ + "/commit", "", job_.maxHttpRetries_, headers);
      }
      else
      {
        success = DoDeletePeer(job_.peers_, job_.peerIndex_, transactionUri_, job_.maxHttpRetries_, headers);
      }
        
      if (!success)
      {
        if (isCommit_)
        {
          LOG(ERROR) << "Cannot commit push transaction on remote peer: " // TODO: add job ID
                     << job_.query_.GetPeer();
        }
          
        return StateUpdate::Failure();
      } 
      else if (isCommit_)
      {
        return StateUpdate::Success();
      }
      else
      {
        return StateUpdate::Failure();
      }
    }

    virtual void Stop(OrthancPluginJobStopReason reason)
    {
    }
  };


  class PushJob::PushBucketsState : public IState
  {
  private:
    const PushJob&                    job_;
    JobInfo&                          info_;
    std::string                       transactionUri_;
    HttpQueriesQueue                  queue_;
    std::unique_ptr<HttpQueriesRunner>  runner_;

    void UpdateInfo()
    {
      size_t scheduledQueriesCount, completedQueriesCount;
      uint64_t uploadedSize, downloadedSize;
      queue_.GetStatistics(scheduledQueriesCount, completedQueriesCount, downloadedSize, uploadedSize);

      info_.SetContent("UploadedSizeMB", ConvertToMegabytes(uploadedSize));
      info_.SetContent("CompletedHttpQueries", static_cast<unsigned int>(completedQueriesCount));

      if (runner_.get() != NULL)
      {
        float speed;
        runner_->GetSpeed(speed);
        info_.SetContent("NetworkSpeedKBs", static_cast<unsigned int>(speed));
      }
            
      // The "2" below corresponds to the "CreateTransactionState"
      // and "FinalState" steps (which prevents division by zero)
      info_.SetProgress(static_cast<float>(1 /* CreateTransactionState */ + completedQueriesCount) / 
                        static_cast<float>(2 + scheduledQueriesCount));
    }

  public:
    PushBucketsState(const PushJob&  job,
                     JobInfo& info,
                     const std::string& transactionUri,
                     const std::vector<TransferBucket>& buckets) :
      job_(job),
      info_(info),
      transactionUri_(transactionUri)
    {
      std::map<std::string, std::string> headers;
      job_.query_.GetHttpHeaders(headers);

      queue_.SetMaxRetries(job.maxHttpRetries_);
      queue_.Reserve(buckets.size());
        
      for (size_t i = 0; i < buckets.size(); i++)
      {
        queue_.Enqueue(new BucketPushQuery(job.cache_, buckets[i], job.query_.GetPeer(),
                                           transactionUri_, i, job.query_.GetCompression(), headers));
      }

      UpdateInfo();
    }
      
    virtual StateUpdate* Step()
    {
      if (runner_.get() == NULL)
      {
        runner_.reset(new HttpQueriesRunner(queue_, job_.threadsCount_));
      }

      HttpQueriesQueue::Status status = queue_.WaitComplete(200);

      UpdateInfo();

      switch (status)
      {
        case HttpQueriesQueue::Status_Running:
          return StateUpdate::Continue();

        case HttpQueriesQueue::Status_Success:
          // Commit transaction on remote peer
          return StateUpdate::Next(new FinalState(job_, info_, transactionUri_, true));

        case HttpQueriesQueue::Status_Failure:
          // Discard transaction on remote peer
          return StateUpdate::Next(new FinalState(job_, info_, transactionUri_, false));

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }        
    }

    virtual void Stop(OrthancPluginJobStopReason reason)
    {
      // Cancel the running download threads
      runner_.reset();
    }
  };


  class PushJob::CreateTransactionState : public IState
  {
  private:
    const PushJob&                job_;
    JobInfo&                      info_;
    std::string                   createTransaction_;
    std::vector<TransferBucket>   buckets_;

  public:
    CreateTransactionState(const PushJob& job,
                           JobInfo& info) :
      job_(job),
      info_(info)
    {
      TransferScheduler scheduler;
      scheduler.ParseListOfResources(job_.cache_, job_.query_.GetResources());

      Json::Value push;      
      scheduler.FormatPushTransaction(push, buckets_,
                                      job.targetBucketSize_, 2 * job.targetBucketSize_,
                                      job_.query_.GetCompression());

      Orthanc::Toolbox::WriteFastJson(createTransaction_, push);

      info_.SetContent("Resources", job_.query_.GetResources());
      info_.SetContent("Peer", job_.query_.GetPeer());
      info_.SetContent("Compression", EnumerationToString(job_.query_.GetCompression()));
      info_.SetContent("TotalInstances", static_cast<unsigned int>(scheduler.GetInstancesCount()));
      info_.SetContent("TotalSizeMB", ConvertToMegabytes(scheduler.GetTotalSize()));
    }

    virtual StateUpdate* Step()
    {
      Json::Value answer;
      std::map<std::string, std::string> headers;
      job_.query_.GetHttpHeaders(headers);

      if (!DoPostPeer(answer, job_.peers_, job_.peerIndex_, URI_PUSH, createTransaction_, job_.maxHttpRetries_, headers))
      {
        LOG(ERROR) << "Cannot create a push transaction to peer \"" 
                   << job_.query_.GetPeer()
                   << "\" (check that it has the transfers accelerator plugin installed)";
        return StateUpdate::Failure();
      } 

      if (answer.type() != Json::objectValue ||
          !answer.isMember(KEY_PATH) ||
          answer[KEY_PATH].type() != Json::stringValue)
      {
        LOG(ERROR) << "Bad network protocol from peer: " << job_.query_.GetPeer();
        return StateUpdate::Failure();
      }

      std::string transactionUri = answer[KEY_PATH].asString();

      return StateUpdate::Next(new PushBucketsState(job_, info_, transactionUri, buckets_));
    }

    virtual void Stop(OrthancPluginJobStopReason reason)
    {
    }
  };


  StatefulOrthancJob::StateUpdate* PushJob::CreateInitialState(JobInfo& info)
  {
    return StateUpdate::Next(new CreateTransactionState(*this, info));
  }
    
    
  PushJob::PushJob(const TransferQuery& query,
                   OrthancInstancesCache& cache,
                   size_t threadsCount,
                   size_t targetBucketSize,
                   unsigned int maxHttpRetries) :
    StatefulOrthancJob(JOB_TYPE_PUSH),
    cache_(cache),
    query_(query),
    threadsCount_(threadsCount),
    targetBucketSize_(targetBucketSize),
    maxHttpRetries_(maxHttpRetries)
  {
    if (!peers_.LookupName(peerIndex_, query_.GetPeer()))
    {
      LOG(ERROR) << "Unknown Orthanc peer: " << query_.GetPeer();
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
    }

    Json::Value serialized;
    query.Serialize(serialized);
    UpdateSerialized(serialized);
  }
}
