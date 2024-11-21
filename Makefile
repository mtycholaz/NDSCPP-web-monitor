SHELL:=/bin/bash

CC=clang++
CFLAGS=-std=c++2a -g3 -O3
LDFLAGS=
LIBS=-lcurl -lpthread -lmicrohttpd -lz
DEPFLAGS=-MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

SOURCES=main.cpp server.cpp
EXECUTABLE=ndscpp
DEPDIR=.deps
AIC_CHECK=.ALLOW_IGNORE_CACHE
OBJECTS:=$(SOURCES:.cpp=.o)
DEPFILES:=$(SOURCES:%.cpp=$(DEPDIR)/%.d)

override DEFINES:=$(if $(ALLOW_IGNORE_CACHE),-DALLOW_IGNORE_CACHE=$(ALLOW_IGNORE_CACHE),)

define helptext =
Makefile for ndscpp

Usage:
	help	Show this help text
	all	Build the ndscpp application
	force	Force a rebuild of all files
	clean	Remove all build artifacts

Note:	the ALLOW_IGNORE_CACHE variable can be used to allow (true) or
	disallow (false) the cache to be ignored using the cached=no query
	parameter. The default value is set in the equally named macro in
	global.h.
	For production scenarios it is recommended to NOT allow cached=no.
	Changing this variable between invocations of make will trigger a
	recompile of all C++ source files.

Examples:
	Build ndscpp using the default concerning cached=no:
	$$ make all

	Build ndscpp with support for cached=no:
	$$ make ALLOW_IGNORE_CACHE=true all

endef

# Detect platform
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
    CFLAGS += -I/opt/homebrew/include/
    LDFLAGS += -L/opt/homebrew/lib
else
    CFLAGS += -I/usr/include/
    LDFLAGS += -L/usr/lib/
endif

.PHONY: help
help:; @ $(info $(helptext)) :

.PHONY: all
.PHONY: phony
.PHONY: force
.PHONY: clear_aic
.PHONY: clean

all: $(EXECUTABLE)
force: clear_aic all


clear_aic: ; rm -f $(AIC_CHECK)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

%.o: %.cpp $(DEPDIR)/%.d | $(DEPDIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) $(DEFINES) -c $< -o $@

$(DEPDIR): ; @mkdir -p $@

$(DEPFILES):


clean: clear_aic
	rm -f $(OBJECTS) $(EXECUTABLE) $(DEPFILES) $

include $(wildcard $(DEPFILES))
