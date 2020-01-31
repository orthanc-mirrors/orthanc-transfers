/**
 * Transfers accelerator plugin for Orthanc
 * Copyright (C) 2018-2020 Osimis S.A., Belgium
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


#include "TransferQuery.h"

#include <Core/OrthancException.h>


namespace OrthancPlugins
{
  TransferQuery::TransferQuery(const Json::Value& body)
  {
    if (body.type() != Json::objectValue ||
        !body.isMember(KEY_RESOURCES) ||
        !body.isMember(KEY_PEER) ||
        !body.isMember(KEY_COMPRESSION) ||
        body[KEY_RESOURCES].type() != Json::arrayValue ||
        body[KEY_PEER].type() != Json::stringValue ||
        body[KEY_COMPRESSION].type() != Json::stringValue)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    peer_ = body[KEY_PEER].asString();
    resources_ = body[KEY_RESOURCES];
    compression_ = StringToBucketCompression(body[KEY_COMPRESSION].asString());

    if (body.isMember(KEY_ORIGINATOR_UUID))
    {
      if (body[KEY_ORIGINATOR_UUID].type() != Json::stringValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
      else 
      {
        hasOriginator_ = true;
        originator_ = body[KEY_ORIGINATOR_UUID].asString();
      }
    }
    else
    {
      hasOriginator_ = false;
    }

    if (body.isMember(KEY_PRIORITY))
    {
      if (body[KEY_PRIORITY].type() != Json::intValue &&
          body[KEY_PRIORITY].type() != Json::uintValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
      else 
      {
        priority_ = body[KEY_PRIORITY].asInt();
      }
    }
    else
    {
      priority_ = 0;
    }
  }


  const std::string& TransferQuery::GetOriginator() const
  {
    if (hasOriginator_)
    {
      return originator_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  void TransferQuery::Serialize(Json::Value& target) const
  {
    target = Json::objectValue;
    target[KEY_PEER] = peer_;
    target[KEY_RESOURCES] = resources_;
    target[KEY_COMPRESSION] = EnumerationToString(compression_);
      
    if (hasOriginator_)
    {
      target[KEY_ORIGINATOR_UUID] = originator_;
    }
  }
}
