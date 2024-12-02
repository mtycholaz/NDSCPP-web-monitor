SHELL:=/bin/bash

CC=clang++
CFLAGS=-std=c++20 -g3 -O3 -Ieffects -I/opt/homebrew/include
LDFLAGS=
LIBS=-lpthread -lz -lavformat -lavcodec -lavutil -lswscale -lswresample

DEPFLAGS=-MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

SOURCES=main.cpp palette.cpp
EXECUTABLE=ndscpp
DEPDIR=.deps
OBJECTS:=$(SOURCES:.cpp=.o)
DEPFILES:=$(SOURCES:%.cpp=$(DEPDIR)/%.d)

define helptext =
Makefile for ndscpp

Usage:
	help	Show this help text
	all	Build the ndscpp application
	clean	Remove all build artifacts

Examples:
	$$ make all

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

all: $(EXECUTABLE)

help:; @ $(info $(helptext)) :

$(EXECUTABLE): $(OBJECTS)
	@echo Linking $@...
	@$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

%.o: %.cpp $(DEPDIR)/%.d | $(DEPDIR)
	@echo Compiling $<...
	@$(CC) $(CFLAGS) $(DEPFLAGS) $(DEFINES) -c $< -o $@

$(DEPDIR): ; @mkdir -p $@

$(DEPFILES):

clean:
	@echo Cleaning build files...
	@rm -f $(OBJECTS) $(EXECUTABLE) $(DEPFILES)

.PHONY: all clean help

include $(wildcard $(DEPFILES))
