# Orthanc - A Lightweight, RESTful DICOM Store
# Copyright (C) 2018-2023 Osimis S.A., Belgium
# Copyright (C) 2024-2025 Orthanc Team SRL, Belgium
# Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
#
# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.


cmake_minimum_required(VERSION 2.8)
project(OrthancTransfers)

set(ORTHANC_PLUGIN_VERSION "mainline")

if (ORTHANC_PLUGIN_VERSION STREQUAL "mainline")
  set(ORTHANC_FRAMEWORK_VERSION "mainline")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "hg")
else()
  set(ORTHANC_FRAMEWORK_VERSION "1.12.1")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "web")
endif()


# Parameters of the build
set(ORTHANC_FRAMEWORK_SOURCE "${ORTHANC_FRAMEWORK_DEFAULT_SOURCE}" CACHE STRING "Source of the Orthanc source code (can be \"hg\", \"archive\", \"web\" or \"path\")")
set(ORTHANC_FRAMEWORK_ARCHIVE "" CACHE STRING "Path to the Orthanc archive, if ORTHANC_FRAMEWORK_SOURCE is \"archive\"")
set(ORTHANC_FRAMEWORK_ROOT "" CACHE STRING "Path to the Orthanc source directory, if ORTHANC_FRAMEWORK_SOURCE is \"path\"")
set(ORTHANC_SDK_VERSION "1.12.4" CACHE STRING "Version of the Orthanc plugin SDK to use, if not using the system version (can be \"1.12.1\", \"1.12.4\")")

# Advanced parameters to fine-tune linking against system libraries
set(USE_SYSTEM_ORTHANC_SDK ON CACHE BOOL "Use the system version of the Orthanc plugin SDK")


# Download and setup the Orthanc framework
include(${CMAKE_SOURCE_DIR}/Resources/Orthanc/CMake/DownloadOrthancFramework.cmake)

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "system")
  if (ORTHANC_FRAMEWORK_USE_SHARED)
    include(FindBoost)
    find_package(Boost COMPONENTS thread)
    
    if (NOT Boost_FOUND)
      message(FATAL_ERROR "Unable to locate Boost on this system")
    endif()
    
    link_libraries(${Boost_LIBRARIES} jsoncpp)
  endif()

  link_libraries(${ORTHANC_FRAMEWORK_LIBRARIES})

  set(USE_SYSTEM_GOOGLE_TEST ON CACHE BOOL "Use the system version of Google Test")
  set(USE_GOOGLE_TEST_DEBIAN_PACKAGE OFF CACHE BOOL "Use the sources of Google Test shipped with libgtest-dev (Debian only)")
  mark_as_advanced(USE_GOOGLE_TEST_DEBIAN_PACKAGE)
  include(${CMAKE_SOURCE_DIR}/Resources/Orthanc/CMake/GoogleTestConfiguration.cmake)

else()
  include(${ORTHANC_FRAMEWORK_ROOT}/../Resources/CMake/OrthancFrameworkParameters.cmake)

  set(ENABLE_LOCALE OFF)         # Disable support for locales (notably in Boost)
  set(ENABLE_GOOGLE_TEST ON)
  set(ENABLE_MODULE_IMAGES OFF)
  set(ENABLE_MODULE_JOBS OFF)
  set(ENABLE_MODULE_DICOM OFF)
  set(ENABLE_ZLIB ON)

  include(${ORTHANC_FRAMEWORK_ROOT}/../Resources/CMake/OrthancFrameworkConfiguration.cmake)
  include_directories(${ORTHANC_FRAMEWORK_ROOT})
endif()


include(${CMAKE_SOURCE_DIR}/Resources/Orthanc/Plugins/OrthancPluginsExports.cmake)


# Check that the Orthanc SDK headers are available
if (STATIC_BUILD OR NOT USE_SYSTEM_ORTHANC_SDK)
  if (ORTHANC_SDK_VERSION STREQUAL "1.12.1")
    include_directories(${CMAKE_SOURCE_DIR}/Resources/Orthanc/Sdk-1.12.1)
  elseif (ORTHANC_SDK_VERSION STREQUAL "1.12.4")
    include_directories(${CMAKE_SOURCE_DIR}/Resources/Orthanc/Sdk-1.12.4)
  else()
    message(FATAL_ERROR "Unsupported version of the Orthanc plugin SDK: ${ORTHANC_SDK_VERSION}")
  endif()
else ()
  CHECK_INCLUDE_FILE_CXX(orthanc/OrthancCDatabasePlugin.h HAVE_ORTHANC_H)
  if (NOT HAVE_ORTHANC_H)
    message(FATAL_ERROR "Please install the headers of the Orthanc plugins SDK")
  endif()
endif()



if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  execute_process(
    COMMAND 
    ${PYTHON_EXECUTABLE} ${ORTHANC_FRAMEWORK_ROOT}/../Resources/WindowsResources.py
    ${ORTHANC_PLUGIN_VERSION} "Transfers accelerator plugin" OrthancTransfers.dll
    "Transfers accelerator plugin for Orthanc"
    ERROR_VARIABLE Failure
    OUTPUT_FILE ${AUTOGENERATED_DIR}/Version.rc
    )

  if (Failure)
    message(FATAL_ERROR "Error while computing the version information: ${Failure}")
  endif()

  set(PLUGIN_RESOURCES ${AUTOGENERATED_DIR}/Version.rc)
endif()



EmbedResources(
  ORTHANC_EXPLORER  ${CMAKE_SOURCE_DIR}/Resources/OrthancExplorer.js
  )

set(FRAMEWORK_SOURCES
  Framework/DicomInstanceInfo.cpp
  Framework/DownloadArea.cpp
  Framework/HttpQueries/DetectTransferPlugin.cpp
  Framework/HttpQueries/HttpQueriesQueue.cpp
  Framework/HttpQueries/HttpQueriesRunner.cpp
  Framework/OrthancInstancesCache.cpp
  Framework/PullMode/BucketPullQuery.cpp
  Framework/PullMode/PullJob.cpp
  Framework/PushMode/ActivePushTransactions.cpp
  Framework/PushMode/BucketPushQuery.cpp
  Framework/PushMode/PushJob.cpp
  Framework/SourceDicomInstance.cpp
  Framework/StatefulOrthancJob.cpp
  Framework/TransferBucket.cpp
  Framework/TransferQuery.cpp
  Framework/TransferScheduler.cpp
  Framework/TransferToolbox.cpp
  Resources/Orthanc/Plugins/OrthancPluginCppWrapper.cpp
  )



add_library(OrthancTransfers SHARED
  Plugin/Plugin.cpp
  Plugin/PluginContext.cpp

  ${FRAMEWORK_SOURCES}
  ${PLUGIN_RESOURCES}
  ${ORTHANC_CORE_SOURCES}
  ${AUTOGENERATED_SOURCES}
  )

message("Setting the version of the library to ${ORTHANC_PLUGIN_VERSION}")

add_definitions(
  -DORTHANC_PLUGIN_VERSION="${ORTHANC_PLUGIN_VERSION}"
  -DHAS_ORTHANC_EXCEPTION=1
  )

set_target_properties(OrthancTransfers PROPERTIES 
  VERSION ${ORTHANC_PLUGIN_VERSION} 
  SOVERSION ${ORTHANC_PLUGIN_VERSION}
  COMPILE_FLAGS -DORTHANC_ENABLE_LOGGING_PLUGIN=1
  )

install(
  TARGETS OrthancTransfers
  RUNTIME DESTINATION lib    # Destination for Windows
  LIBRARY DESTINATION share/orthanc/plugins    # Destination for Linux
  )

add_executable(UnitTests
  UnitTests/UnitTestsMain.cpp

  ${FRAMEWORK_SOURCES}
  ${ORTHANC_CORE_SOURCES}
  ${GOOGLE_TEST_SOURCES}
  )

target_link_libraries(UnitTests ${GOOGLE_TEST_LIBRARIES})
set_target_properties(UnitTests PROPERTIES
  COMPILE_FLAGS -DORTHANC_ENABLE_LOGGING_PLUGIN=0
  )

if (COMMAND DefineSourceBasenameForTarget)
  DefineSourceBasenameForTarget(OrthancTransfers)
  DefineSourceBasenameForTarget(UnitTests)
endif()
