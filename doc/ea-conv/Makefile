#
# Copyright (c) 2000, 2002 Silicon Graphics, Inc.  All Rights Reserved.
#

TOPDIR = ../..
include $(TOPDIR)/include/builddefs

LSRCFILES = README ea-conv

include $(BUILDRULES)

install: default
	$(INSTALL) -m 755 -d $(PKG_DOC_DIR)/ea-conv
	$(INSTALL) -m 644 README $(PKG_DOC_DIR)/ea-conv
	$(INSTALL) -m 755 ea-conv $(PKG_DOC_DIR)/ea-conv

default install-dev install-lib:
