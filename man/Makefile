#
# Copyright (c) 2000-2002 Silicon Graphics, Inc.  All Rights Reserved.
#

TOPDIR = ..
include $(TOPDIR)/include/builddefs

SUBDIRS = man1 man2 man3 man5

default : $(SUBDIRS)

install : $(addsuffix -install,$(SUBDIRS))

install-dev : $(addsuffix -install-dev,$(SUBDIRS))

install-lib : $(addsuffix -install-lib,$(SUBDIRS))

%-install:
	$(MAKE) -C $* install

%-install-dev:
	$(MAKE) -C $* install-dev

%-install-lib:
	$(MAKE) -C $* install-lib

include $(BUILDRULES)
