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

#include <stdint.h>
#include <string>

static const unsigned int KB = 1024;
static const unsigned int MB = 1024 * 1024;

static const char* const JOB_TYPE_PULL = "PullTransfer";
static const char* const JOB_TYPE_PUSH = "PushTransfer";

static const char* const PLUGIN_NAME = "transfers";

static const char* const KEY_BIDIRECTIONAL_PEERS = "BidirectionalPeers";
static const char* const KEY_BUCKETS = "Buckets";
static const char* const KEY_COMPRESSION = "Compression";
static const char* const KEY_ID = "ID";
static const char* const KEY_INSTANCES = "Instances";
static const char* const KEY_LEVEL = "Level";
static const char* const KEY_OFFSET = "Offset";
static const char* const KEY_ORIGINATOR_UUID = "Originator";
static const char* const KEY_PATH = "Path";
static const char* const KEY_PEER = "Peer";
static const char* const KEY_PLUGIN_CONFIGURATION = "Transfers";
static const char* const KEY_PRIORITY = "Priority";
static const char* const KEY_REMOTE_JOB = "RemoteJob";
static const char* const KEY_RESOURCES = "Resources";
static const char* const KEY_SIZE = "Size";
static const char* const KEY_URL = "URL";

static const char* const URI_CHUNKS = "/transfers/chunks";
static const char* const URI_JOBS = "/jobs";
static const char* const URI_LOOKUP = "/transfers/lookup";
static const char* const URI_PEERS = "/transfers/peers";
static const char* const URI_PLUGINS = "/plugins";
static const char* const URI_PULL = "/transfers/pull";
static const char* const URI_PUSH = "/transfers/push";
static const char* const URI_SEND = "/transfers/send";

  
namespace OrthancPlugins
{
  enum BucketCompression
  {
    BucketCompression_None,
    BucketCompression_Gzip
  };

  enum PeerCapabilities
  {
    PeerCapabilities_Disabled,
    PeerCapabilities_Installed,
    PeerCapabilities_Bidirectional
  };

  unsigned int ConvertToMegabytes(uint64_t value);

  unsigned int ConvertToKilobytes(uint64_t value);

  BucketCompression StringToBucketCompression(const std::string& value);

  const char* EnumerationToString(BucketCompression compression);
}
