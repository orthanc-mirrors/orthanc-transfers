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


#include "SourceDicomInstance.h"

#include <Core/Logging.h>
#include <Core/Toolbox.h>
#include <Plugins/Samples/Common/OrthancPluginCppWrapper.h>


namespace OrthancPlugins
{
  SourceDicomInstance::SourceDicomInstance(OrthancPluginContext* context,
                                           const std::string& instanceId) :
    context_(context)
  {
    LOG(INFO) << "Transfers accelerator reading DICOM instance: " << instanceId;
      
    MemoryBuffer buffer(context);
    buffer.GetDicomInstance(instanceId);

    info_.reset(new DicomInstanceInfo(instanceId, buffer));

    buffer_ = buffer.Release();
  }

  
  SourceDicomInstance::~SourceDicomInstance()
  {
    OrthancPluginFreeMemoryBuffer(context_, &buffer_);
  }


  const DicomInstanceInfo& SourceDicomInstance::GetInfo() const
  {
    assert(info_.get() != NULL);
    return *info_;
  }


  void SourceDicomInstance::GetChunk(std::string& target /* out */,
                                     std::string& md5 /* out */,
                                     size_t offset,
                                     size_t size) const
  {
    if (offset + size > buffer_.size)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    const char* start = reinterpret_cast<const char*>(buffer_.data) + offset;
    target.assign(start, start + size);

    Orthanc::Toolbox::ComputeMD5(md5, start, size);
  }
}
