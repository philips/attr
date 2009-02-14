#
# Copyright (c) 2001-2002 Silicon Graphics, Inc.  All Rights Reserved.
#

TOPDIR = ..
include $(TOPDIR)/include/builddefs

TEST = $(wildcard *.test)
EXT = $(wildcard ext/*.test)

# ensure we pick these up in the source tarball
LSRCFILES = $(TEST) $(EXT) run README

default install install-dev install-lib:

include $(BUILDRULES)

PATH := $(abspath ../getfattr/):$(abspath ../setfattr):$(abspath ../chattr):$(PATH)

tests: $(TEST)
ext-tests: $(EXT)

$(TEST):
	@echo "*** $@ ***"; perl run $@

$(EXT):
	@echo "EXT specific tests"; @echo "*** $@ ***"; perl run $@

.PHONY: $(TEST) $(EXT)
.NOTPARALLEL:
