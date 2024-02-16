.POSIX:
.SUFFIXES:
.PHONY: all install uninstall run prep install_deps clean

CXX=g++
CXXFLAGS=-Wall -Wextra -Wpedantic -Ofast --std=c++17 -mtune=native -march=native
CXXEXTRAS=
CXXFLAGS+=$(CXXEXTRAS)
LDLIBS=-lpthread

# for BSD make or bmake(1) on linux
RM ?= rm -f

INCLUDE_DIR=include
PREFIX=$(HOME)/.local

PROJECT=numbers

NBLIC=https://raw.githubusercontent.com/martinus/nanobench/v4.3.11/LICENSE
NB=https://raw.githubusercontent.com/martinus/nanobench/v4.3.11/src/include/nanobench.h
ARGHLIC=https://raw.githubusercontent.com/adishavit/argh/master/LICENSE
ARGH=https://raw.githubusercontent.com/adishavit/argh/master/argh.h

$(PROJECT): numbers.o
	$(CXX) -o $(PROJECT) numbers.o $(LDLIBS)
all: $(PROJECT)
install: $(PROJECT)
	mkdir -p $(PREFIX)/bin
	cp -f $(PROJECT) $(PREFIX)/bin/$(PROJECT)
	@echo "numbers installed at:" $(PREFIX)/bin/numbers
	@echo "run make uninstall to remove numbers."
uninstall:
	$(RM) $(PREFIX)/bin/$(PROJECT)
run: $(PROJECT)
	$(PWD)/numbers
prep:
	@if [ ! -d $(PWD)/$(BUILD_DIR) ]; then mkdir -p $(PWD)/$(BUILD_DIR); fi;
install_deps: prep
	wget --quiet -O $(INCLUDE_DIR)/LICENSE $(NBLIC)
	wget --quiet -O $(INCLUDE_DIR)/nanobench.h $(NB)
	wget --quiet -O $(INCLUDE_DIR)/LICENSE.3bsd $(ARGHLIC)
	wget --quiet -O $(INCLUDE_DIR)/argh.h $(ARGH)
clean:
	@$(RM) *.o
.SUFFIXES: .o .cc
.cc.o:
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $<
-include workflow.mk # private
