#
# Copyright (c) 2001-2002 Silicon Graphics, Inc.  All Rights Reserved.
#

TOPDIR = ..
include $(TOPDIR)/include/builddefs

TEST = $(wildcard *.test)
EXT = $(wildcard ext/*.test)
ROOT = $(wildcard root/*.test)

# ensure we pick these up in the source tarball
LSRCFILES = $(TEST) $(EXT) $(ROOT) run README

default install install-dev install-lib:

include $(BUILDRULES)

PATH := $(abspath ../getfattr/):$(abspath ../setfattr):$(abspath ../chattr):$(PATH)

tests: $(TEST)
ext-tests: $(EXT)
root-tests: $(ROOT)

$(TEST):
	@echo "*** $@ ***"; perl run $@

$(EXT):
	@echo "EXT specific tests"; @echo "*** $@ ***"; perl run $@

$(ROOT):
	@echo "Note: Tests must run as root"; @echo "*** $@ ***"; perl run $@

.PHONY: $(TEST) $(EXT) $(ROOT)
.NOTPARALLEL:
