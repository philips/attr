#
# Copyright (c) 2001-2002 Silicon Graphics, Inc.  All Rights Reserved.
#

TOPDIR = ..
include $(TOPDIR)/include/builddefs

LINGUAS = de fr nl pl sv
LSRCFILES = $(LINGUAS:%=%.po) $(PKG_NAME).pot

XGETTEXTFILES =	$(TOPDIR)/attr/attr.c \
		$(TOPDIR)/getfattr/getfattr.c \
		$(TOPDIR)/setfattr/setfattr.c \
		$(TOPDIR)/libattr/attr_copy_fd.c \
		$(TOPDIR)/libattr/attr_copy_file.c

default: $(LINGUAS:%=%.mo)

include $(BUILDRULES)

install: default
	$(INSTALL_LINGUAS)

install-dev install-lib:
