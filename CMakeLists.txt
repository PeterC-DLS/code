## Process this file with cmake
#=============================================================================
#  NeXus - Neutron & X-ray Common Data Format
#
#  CMakeLists for building the NeXus library and applications.
#
# Copyright (C) 2008-2012 NeXus International Advisory Committee (NIAC)
#
#  This library is free software; you can redistribute it and/or modify it
#  under the terms of the GNU Lesser General Public License as published by the
#  Free Software Foundation; either version 2 of the License, or (at your
#  option) any later version.
#
#  This library is distributed in the hope that it will be useful, but WITHOUT
#  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
#  for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with this library; if not, write to the Free Software Foundation,
#  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#  For further information, see <http://www.nexusformat.org>
#
#==============================================================================

cmake_minimum_required(VERSION 2.8.7)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

#The name of our project
project (NeXus)
enable_testing()

#set the module path
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake_include")

#set some custom targets
add_custom_target(html)

#-----------------------------------------------------------------------------
# setup installation directories
#
# This is an OS specific task and we need to take care about the target 
# architecture. 
#-----------------------------------------------------------------------------
if(CMAKE_HOST_UNIX)
    #easy on Unix - just follow the GNU standard
    include(GNUInstallDirs)
endif()


#-----------------------------------------------------------------------------
# define package version
#-----------------------------------------------------------------------------
set (NEXUS_VERSION_MAJOR 4)
set (NEXUS_VERSION_MINOR 4)
set (NEXUS_VERSION_PATCH 0)
set (NEXUS_SONAME 4)
set (NAPI_VERSION "${NEXUS_VERSION_MAJOR}.${NEXUS_VERSION_MINOR}.${NEXUS_VERSION_PATCH}")
set (NXLTVERSINFO "${NEXUS_VERSION_MAJOR}:${NEXUS_VERSION_MINOR}:${NEXUS_VERSION_PATCH}")

#-----------------------------------------------------------------------------
# define command line options by which the user can configure the build
#-----------------------------------------------------------------------------
option(ENABLE_HDF5 "Build with HDF5 support" ON)
option(ENABLE_HDF4 "Build with HDF4 support" OFF)
option(ENABLE_MXML "Build with XML support" OFF)
option(ENABLE_CONTRIB "Build the user contributions" OFF)
option(ENABLE_FORTRAN90 "Build the Fortran 90 bindings" OFF)
option(ENABLE_FORTRAN77 "Build the Fortran 77 bindings" OFF)
option(ENABLE_JAVA      "Build Java bindings" OFF)
option(ENABLE_CXX       "Build C++ bindings" OFF)
option(ENABLE_APPS      "Build utility applications" OFF)

#show this only on Windows systems
if(CMAKE_HOST_WIN32)
    option(MINGW_MSYS   "We are building under MINGW." OFF)
    option(HAVE_MS_LIB  "We are building under MINGW." OFF)
endif()

#-----------------------------------------------------------------------------
# include some general purpose modules
#-----------------------------------------------------------------------------
include(Utilities)
include(CompilerChecks)


#-----------------------------------------------------------------------------
# check for package config
#-----------------------------------------------------------------------------
include(FindPkgConfig)

#include(cmake_include/FindCBFLib.cmake)
#include(cmake_include/FindJava.cmake)
#include(cmake_include/FindJNI.cmake)
#include(cmake_include/FindGuile.cmake)
#include(cmake_include/FindMZScheme.cmake)
#include(cmake_include/FindIDL.cmake)

#------------------------------------------------------------------------------
# a list of libraries we have to link NAPI against
# we need this in the global scope as all other programs have to be
# linked against this libraries as well - in this early stage there is
# no pkg-config we could use!
#------------------------------------------------------------------------------
set(NAPI_LINK_LIBS)

if(ENABLE_MXML)
    include(FindMXML)
    # the required libraries are appended to
    # NAPI_LINK_LIBS by this module
    set(WITH_MXML TRUE)
else()
    #in all cases we build the libary without MXML
    message(STATUS "Build without MXML support!")
endif()

if(ENABLE_HDF4)
    include(FindHDF4)

    message(STATUS "HDF4 library found in: ${HDF4_LIBRARY_DIR}")
    message(STATUS "HDF4 headers found in: ${HDF4_INCLUDE_DIR}")

    # the required libraries are appended to
    # NAPI_LINK_LIBS by this module
    set(WITH_HDF4 TRUE)
else()
    message(STATUS "Build without HDF4 support!")
endif()

if(ENABLE_HDF5)

    message (STATUS "${HDF5_COMPONENTS}")
    if (ENABLE_CXX)
        find_package ( HDF5 COMPONENTS CXX HL REQUIRED )
    else()
        find_package ( HDF5 REQUIRED )
    endif()

    # Hide annoying and confusing "HDF5_DIR-NOTFOUND" in CMake-GUI
    if (HDF5_DIR STREQUAL "HDF5_DIR-NOTFOUND")
      unset (HDF5_DIR CACHE)
    endif()

    set(HAVE_HDF5 1)
    list(APPEND NAPI_LINK_LIBS ${HDF5_LIBRARIES})
    set(WITH_HDF5 TRUE)

    message(STATUS "HDF5 library found in: ${HDF5_LIBRARY_DIRS}")
    message(STATUS "HDF5 headers found in: ${HDF5_INCLUDE_DIRS}")
else()
    message(STATUS "Build without HDF5 support!")
endif()

if(ENABLE_FORTRAN90 OR ENABLE_FORTRAN77)
    enable_language(Fortran)
endif()

#-----------------------------------------------------------------------------
# if contributed programs are built we have to add JPEG as a requirement 
# for nxextract
#-----------------------------------------------------------------------------
if(ENABLE_CONTRIB)
    find_package(JPEG REQUIRED)
endif()

#include(cmake_include/FindOpenGenie.cmake)
#include(cmake_include/FindSZIP.cmake)
#include(cmake_include/FindAnt.cmake)

#find_package(XMLRPC)
find_package(ZLIB)
find_package(LibXml2)
find_package(Doxygen)
#find_package(SWIG)
#find_package(TCL)
#find_package(LATEX)
#find_package(PythonInterp)

#Find the java runtime and sdk
#if(ENABLE_JAVA_BINDINGS)
#    find_package(Java 1.6)
#    find_package(JNI)
#endif()


find_library(PTHREAD pthread)
if(PTHREAD)
   set(PTHREAD_LINK "-lpthread")

   #this fixes an issue on OpenSuse 13.2 where the MXML library is not 
   #prelinked with threads
   if(WITH_MXML)
       list(APPEND NAPI_LINK_LIBS ${PTHREAD_LINK})
   endif()
endif(PTHREAD)

include_directories("${PROJECT_BINARY_DIR}/include"
                    "${PROJECT_SOURCE_DIR}/include")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${NX_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NX_CFLAGS}")

# Recurse into the subdirectories.
add_subdirectory (include)
add_subdirectory (src)
add_subdirectory (test)
add_subdirectory (bindings)

#on user request utility applications will be built
if(ENABLE_APPS)
    add_subdirectory (applications)
endif()

add_subdirectory (doc)
#add_subdirectory (config)

if(ENABLE_CONTRIB)
    add_subdirectory (contrib)
endif()

#add_subdirectory (InstallerBits)
#add_subdirectory (macosx_install_kit)
#add_subdirectory (scripts)
#add_subdirectory (third_party)
#add_subdirectory (windows)
#add_subdirectory (Windows_extra)
