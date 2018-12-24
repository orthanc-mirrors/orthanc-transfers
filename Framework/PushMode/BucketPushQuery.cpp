/**
 * Transfers accelerator plugin for Orthanc
 * Copyright (C) 2018-2019 Osimis S.A., Belgium
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


#include "BucketPushQuery.h"

#include <Core/ChunkedBuffer.h>
#include <Core/Compression/GzipCompressor.h>

#include <boost/lexical_cast.hpp>


namespace OrthancPlugins
{
  BucketPushQuery::BucketPushQuery(OrthancInstancesCache& cache,
                                   const TransferBucket& bucket,
                                   const std::string& peer,
                                   const std::string& transactionUri,
                                   size_t bucketIndex,
                                   BucketCompression compression) :
    cache_(cache),
    bucket_(bucket),
    peer_(peer),
    compression_(compression)
  {
    uri_ = transactionUri + "/" + boost::lexical_cast<std::string>(bucketIndex);
  }


  void BucketPushQuery::ReadBody(std::string& body) const
  {
    Orthanc::ChunkedBuffer buffer;

    for (size_t j = 0; j < bucket_.GetChunksCount(); j++)
    {
      std::string chunk;
      std::string md5;  // unused
      cache_.GetChunk(chunk, md5, bucket_, j);
      buffer.AddChunk(chunk);
    }

    switch (compression_)
    {
      case BucketCompression_None:
        buffer.Flatten(body);
        break;

      case BucketCompression_Gzip:
      {
        std::string raw;
        buffer.Flatten(raw);
            
        Orthanc::GzipCompressor compressor;
        Orthanc::IBufferCompressor::Compress(body, compressor, raw);
        break;
      }

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }

  
  void BucketPushQuery::HandleAnswer(const void* answer,
                                     size_t size)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }
}
