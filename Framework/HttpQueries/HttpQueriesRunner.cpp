/**
 * Transfers accelerator plugin for Orthanc
 * Copyright (C) 2018-2023 Osimis S.A., Belgium
 * Copyright (C) 2024-2026 Orthanc Team SRL, Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#include "HttpQueriesRunner.h"

#include <OrthancException.h>
#include <Logging.h>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>


namespace OrthancPlugins
{
  static boost::mutex httpThreadsCounterMutex;
  static uint32_t httpThreadsCounter = 0;

 
  void HttpQueriesRunner::Worker(HttpQueriesRunner* that)
  {
    {
      boost::mutex::scoped_lock lock(httpThreadsCounterMutex);
      Orthanc::Logging::SetCurrentThreadName(std::string(that->threadNamePrefix_) + boost::lexical_cast<std::string>(httpThreadsCounter++));
      httpThreadsCounter %= 1000000;
    }

    while (that->continue_)
    {
      size_t size;
        
      if (that->queue_.ExecuteOneQuery(size))
      {
        boost::mutex::scoped_lock lock(that->mutex_);
        that->totalTraffic_ += size;
        that->lastUpdate_ = boost::posix_time::microsec_clock::local_time();
      }
      else
      {
        // We're done (either failure, or no more pending queries)
        return;
      }
    }
  }


  HttpQueriesRunner::HttpQueriesRunner(HttpQueriesQueue& queue,
                                       size_t threadsCount,
                                       const char* threadNamePrefix10charMax) :
    queue_(queue),
    continue_(true),
    start_(boost::posix_time::microsec_clock::local_time()),
    totalTraffic_(0),
    lastUpdate_(start_),
    threadNamePrefix_(threadNamePrefix10charMax)
  {
    if (threadsCount == 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
      
    if (strlen(threadNamePrefix_) > 10)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    workers_.resize(threadsCount);

    for (size_t i = 0; i < threadsCount; i++)
    {
      workers_[i] = new boost::thread(Worker, this);
    }
  }


  HttpQueriesRunner::~HttpQueriesRunner()
  {
    continue_ = false;

    for (size_t i = 0; i < workers_.size(); i++)
    {
      if (workers_[i] != NULL)
      {
        if (workers_[i]->joinable())
        {
          workers_[i]->join();
        }

        delete workers_[i];
      }
    }
  }

    
  void HttpQueriesRunner::GetSpeed(float& kilobytesPerSecond)
  {
    boost::mutex::scoped_lock lock(mutex_);
      
    double ms = static_cast<double>((lastUpdate_ - start_).total_milliseconds());

    if (ms < 10.0)
    {
      // Prevents division by zero on very quick transfers
      kilobytesPerSecond = 0;
    }
    else
    {
      kilobytesPerSecond = static_cast<float>(
        static_cast<double>(totalTraffic_) * 1000.0 /*ms*/ / (1024.0 /*KB*/ * ms));
    }
  }
}
