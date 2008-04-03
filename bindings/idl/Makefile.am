## Process this file with automake to produce Makefile.in
#====================================================================
#  NeXus - Neutron & X-ray Common Data Format
#  
#  $Id: Makefile.am 715 2005-12-16 18:11:19Z faa59 $
#  
#  Makefile for NeXus FORTRAN 90 bindings
#
#  Copyright (C) 2004 Freddie Akeroyd
#  
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
# 
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
# 
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free 
#  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
#  MA  02111-1307  USA
#             
#  For further information, see <http://www.neutron.anl.gov/NeXus/>
#
#
#====================================================================
#
# Unix makefile for NeXusIDL-API DLM.
#
# This makefile is used to build NeXusIDL-API --- an example DLM.
# It works by determining what platform you are on and then
# recursively invoking make with the proper options to do the build.
#
lib_LTLIBRARIES = libNeXusIDL-API.la

libNeXusIDL_API_la_SOURCES = NeXusIDL-API.c handle.c handle.h
libNeXusIDL_API_la_LIBADD = $(top_builddir)/src/libNeXus.la
libNeXusIDL_API_la_LDFLAGS = @SHARED_LDFLAGS@ -Wl,--no-undefined -L$(IDLROOT)/bin/bin.linux.x86 -lidl $(LDFLAGS)

AM_CPPFLAGS = -I$(IDLROOT)/external/include

EXTRA_DIST = \
 build_testmodule.pro \
 build_win.bat \
 data \
 NeXusIDL-API.def \
 NeXusIDL-API.dlm \
 NeXusIDL-API.export \
 nxext.h5 \
 nxext.hdf \
 nxext.xml \
 NXtest.h5 \
 NXtest.hdf \
 NXtest.xml \
 README.html \
 read_test.pro \
 recursiveread.pro \
 recursivesearch.pro \
 testfocus.pro \
 write_test.pro
include $(top_srcdir)/build_rules.am