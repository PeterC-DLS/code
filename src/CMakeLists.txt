## Process this file with cmake
#=============================================================================
#  NeXus - Neutron & X-ray Common Data Format
#
#  CMakeLists for building the NeXus library and applications.
#
#  Copyright (C) 2011 Stephen Rankin
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
#
#=============================================================================

add_definitions(-DIN_NEXUS_LIBRARY=1 ${NX_CPP})

#-----------------------------------------------------------------------------
# generate list of common source files
#-----------------------------------------------------------------------------
set (NAPISRC napi.c napiu.c nxstack.c nxstack.h stptok.c  nxdataset.c 
             nxdataset.h nx_stptok.h)

if (ENABLE_FORTRAN90 OR ENABLE_FORTRAN77)
  set (NAPISRC ${NAPISRC} napi_fortran_helper.c)
endif(ENABLE_FORTRAN90 OR ENABLE_FORTRAN77)

set (NAPILINK)

#-----------------------------------------------------------------------------
# add HDF4 specific source files
#-----------------------------------------------------------------------------
if (WITH_HDF4)
  set (NAPISRC ${NAPISRC} napi4.c)
  include_directories(${HDF4_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR})
  link_directories(${HDF4_LIBRARY_DIRS})
endif(WITH_HDF4)

#-----------------------------------------------------------------------------
# add HDF5 specific source files
#-----------------------------------------------------------------------------
if (WITH_HDF5)
  set (NAPISRC ${NAPISRC} napi5.c)
  include_directories(${HDF5_INCLUDE_DIRS})
  link_directories(${HDF5_LIBRARY_DIRS})
endif (WITH_HDF5)

#-----------------------------------------------------------------------------
# add XML specific source files
#-----------------------------------------------------------------------------
if(WITH_MXML)
    set (NAPISRC ${NAPISRC} nxxml.c nxio.c nxio.h)
	include_directories(${MXML_INCLUDE_DIRS})
    link_directories(${MXML_LIBRARY_DIRS})
endif(WITH_MXML)

set_property(SOURCE ${NAPISRC} APPEND PROPERTY COMPILE_FLAGS ${NX_CFLAGS})

#------------------------------------------------------------------------------
# building the static library
#------------------------------------------------------------------------------
add_library (NeXus_Static_Library STATIC ${NAPISRC})

set_target_properties(NeXus_Static_Library PROPERTIES 
                      OUTPUT_NAME NeXus)

target_link_libraries(NeXus_Static_Library ${NAPI_LINK_LIBS})
#------------------------------------------------------------------------------
# building the shared library
#------------------------------------------------------------------------------
add_library (NeXus_Shared_Library SHARED ${NAPISRC})

set_target_properties(NeXus_Shared_Library PROPERTIES 
                      OUTPUT_NAME NeXus
                      VERSION ${NAPI_VERSION}
                      SOVERSION 4)

target_link_libraries(NeXus_Shared_Library ${NAPI_LINK_LIBS})


#------------------------------------------------------------------------------
# install the targets
#-----------------------------------------------------------------------------
install (TARGETS NeXus_Shared_Library
         DESTINATION ${CMAKE_INSTALL_LIBDIR}
         COMPONENT Runtime)

install (TARGETS NeXus_Static_Library 
         DESTINATION ${CMAKE_INSTALL_LIBDIR}
         COMPONENT Development)

