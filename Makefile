.POSIX:
.SUFFIXES:
.PHONY: all install uninstall numbers prep install_deps clean run

CXX=g++
CXXFLAGS=-Wall -Wextra -Wpedantic -Ofast --std=c++17 -mtune=native -march=native
CXXEXTRAS=
CXXFLAGS+=$(CXXEXTRAS)
LDLIBS=-lpthread

# for BSD make or bmake(1) on linux
RM ?= rm -f

SRC_DIR=src
BUILD_DIR=build
INCLUDE_DIR=include
PREFIX=$(HOME)/.local

PROJECT=numbers

NBLIC=https://raw.githubusercontent.com/martinus/nanobench/v4.3.0/LICENSE
NB=https://raw.githubusercontent.com/martinus/nanobench/v4.3.0/src/include/nanobench.h
ARGHLIC=https://raw.githubusercontent.com/adishavit/argh/master/LICENSE
ARGH=https://raw.githubusercontent.com/adishavit/argh/master/argh.h

VPATH = $(SRC_DIR):$(INCLUDE_DIR)

all: prep $(PROJECT)
install: $(PROJECT)
	mkdir -p $(PREFIX)/bin
	cp -f $(BUILD_DIR)/$(PROJECT) $(PREFIX)/bin/$(PROJECT)
	@echo "numbers installed at:" $(PREFIX)/bin/numbers
	@echo "run make uninstall to remove numbers."
run: install
	$(PREFIX)/bin/numbers
uninstall:
	$(RM) $(PREFIX)/bin/$(PROJECT)
$(PROJECT): nanobench.o numbers.o
	$(CXX) $(LDFLAGS) -o $(BUILD_DIR)/$(PROJECT) $(BUILD_DIR)/*.o $(LDLIBS)
prep:
	@if [ ! -d $(PWD)/$(BUILD_DIR) ]; then mkdir -p $(PWD)/$(BUILD_DIR); fi;
install_deps:
	wget --quiet -O $(INCLUDE_DIR)/LICENSE $(NBLIC)
	wget --quiet -O $(INCLUDE_DIR)/nanobench.h $(NB)
	wget --quiet -O LICENSE.3bsd $(ARGHLIC)
	wget --quiet -O $(INCLUDE_DIR)/argh.h $(ARGH)
clean:
	@$(RM) $(BUILD_DIR)/*
.SUFFIXES: .o .cc
.cc.o:
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $(BUILD_DIR)/$@
-include workflow.mk # private
