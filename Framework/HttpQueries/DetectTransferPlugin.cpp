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


#include "DetectTransferPlugin.h"

#include "../TransferToolbox.h"
#include "HttpQueriesRunner.h"

#include <Core/OrthancException.h>

#include <json/reader.h>


namespace OrthancPlugins
{
  DetectTransferPlugin::DetectTransferPlugin(Result&  result,
                                             const std::string& peer) :
    result_(result),
    peer_(peer),
    uri_(URI_PLUGINS)
  {
    result_[peer_] = false;
  }


  void DetectTransferPlugin::ReadBody(std::string& body) const
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }


  void DetectTransferPlugin::HandleAnswer(const void* answer,
                                          size_t size)
  {
    Json::Reader reader;
    Json::Value value;

    if (reader.parse(reinterpret_cast<const char*>(answer), 
                     reinterpret_cast<const char*>(answer) + size, value) &&
        value.type() == Json::arrayValue)
    {
      for (Json::Value::ArrayIndex i = 0; i < value.size(); i++)
      {
        if (value[i].type() == Json::stringValue &&
            value[i].asString() == PLUGIN_NAME)
        {
          result_[peer_] = true;
        }
      }
    }
  }


  void DetectTransferPlugin::Apply(Result& result,
                                   size_t threadsCount,
                                   unsigned int timeout)
  {
    OrthancPlugins::HttpQueriesQueue queue;

    queue.GetOrthancPeers().SetTimeout(timeout);
    queue.Reserve(queue.GetOrthancPeers().GetPeersCount());

    for (size_t i = 0; i < queue.GetOrthancPeers().GetPeersCount(); i++)
    {
      queue.Enqueue(new OrthancPlugins::DetectTransferPlugin
                    (result, queue.GetOrthancPeers().GetPeerName(i)));
    }

    {
      OrthancPlugins::HttpQueriesRunner runner(queue, threadsCount);
      queue.WaitComplete();
    }
  }
}
