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


#pragma once

#include "DicomInstanceInfo.h"

#include <orthanc/OrthancCPlugin.h>

#include <boost/noncopyable.hpp>
#include <memory>

namespace OrthancPlugins
{
  class SourceDicomInstance : public boost::noncopyable
  {
  private:
    OrthancPluginContext*             context_;
    OrthancPluginMemoryBuffer         buffer_;
    std::auto_ptr<DicomInstanceInfo>  info_;

  public:
    SourceDicomInstance(OrthancPluginContext* context,
                        const std::string& instanceId);

    ~SourceDicomInstance();

    const void* GetBuffer() const
    {
      return buffer_.data;
    }

    const DicomInstanceInfo& GetInfo() const;

    void GetChunk(std::string& target /* out */,
                  std::string& md5 /* out */,
                  size_t offset,
                  size_t size) const;
  };
}
