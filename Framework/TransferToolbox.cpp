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


#include "TransferToolbox.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/math/special_functions/round.hpp>


namespace OrthancPlugins
{
  unsigned int ConvertToMegabytes(uint64_t value)
  {
    return static_cast<unsigned int>
      (boost::math::round(static_cast<float>(value) / static_cast<float>(MB)));
  }


  unsigned int ConvertToKilobytes(uint64_t value)
  {
    return static_cast<unsigned int>
      (boost::math::round(static_cast<float>(value) / static_cast<float>(KB)));
  }


  BucketCompression StringToBucketCompression(const std::string& value)
  {
    if (value == "gzip")
    {
      return BucketCompression_Gzip;
    }
    else if (value == "none")
    {
      return BucketCompression_None;
    }
    else
    {
      LOG(ERROR) << "Valid compression methods are \"gzip\" and \"none\", but found: " << value;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  const char* EnumerationToString(BucketCompression compression)
  {
    switch (compression)
    {
      case BucketCompression_Gzip:
        return "gzip";

      case BucketCompression_None:
        return "none";
        
      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }
}
