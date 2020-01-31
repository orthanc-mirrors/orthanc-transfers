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


#pragma once

#include "TransferToolbox.h"

#include <json/value.h>

namespace OrthancPlugins
{
  class TransferQuery
  {
  private:
    std::string        peer_;
    Json::Value        resources_;
    BucketCompression  compression_;
    bool               hasOriginator_;
    std::string        originator_;
    int                priority_;

  public:
    explicit TransferQuery(const Json::Value& body);

    const std::string& GetPeer() const
    {
      return peer_;
    }

    BucketCompression GetCompression() const
    {
      return compression_;
    }

    const Json::Value& GetResources() const
    {
      return resources_;
    }

    bool HasOriginator() const
    {
      return hasOriginator_;
    }

    const std::string& GetOriginator() const;

    int GetPriority() const
    {
      return priority_;
    }

    void Serialize(Json::Value& target) const;
  };
}
