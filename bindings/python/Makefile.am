## Process this file with automake to produce Makefile.in
#====================================================================
#  NeXus - Neutron & X-ray Common Data Format
#  
#  $Id: Makefile.am 715 2005-12-16 18:11:19Z faa59 $
#  
#  Makefile for NeXus python bindings
#
#  Copyright (C) 2008 Freddie Akeroyd
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
#  For further information, see <http://www.nexusformat.org>
#
#
#====================================================================

pydocdir 	= $(NXDOCDIR)/python
pydoc_DATA	= README.html nxs.napi.html
#nxspythondir	= $(pkgpythondir)/nxs
nxspythondir	= $(pythondir)/nxs
#pkgpython_PYTHON = nxstest.py
nxspython_PYTHON = nxs/__init__.py nxs/napi.py nxs/tree.py nxs/unit.py
EXTRA_DIST	= README.html nxs.napi.html nxstest.py run_nxstest setup.py

nxs.napi.html : $(srcdir)/nxs/napi.py
	env NEXUSLIB=../../src/.libs/libNeXus.$(SHARED_EXT) pydoc -w $(srcdir)/nxs/napi.py
	if test -r napi.html; then \
	    mv napi.html $@; \
	else \
	    touch $@; \
	fi

install-data-hook :
	sed -e "s|nxlibdir = .*|nxlibdir = \'${libdir}\'|" < ${srcdir}/nxs/napi.py > $(DESTDIR)${nxspythondir}/napi.py
	if test -z "$(DESTDIR)"; then \
	    $(py_compile) --basedir "$(nxspythondir)" napi.py; \
	else \
	    $(py_compile) --destdir "$(DESTDIR)" --basedir "$(nxspythondir)" napi.py; \
	fi

# python setup.py install --root=$(DESTDIR)
CLEANFILES=nxs.napi.html nxs/napi.pyc

include $(top_srcdir)/build_rules.am
