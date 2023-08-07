#
# Copyright (C) 2023 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

.PRECIOUS: %.cpp

SYSTEM   = $(shell uname | tr '[:upper:]' '[:lower:]')
SYS_ARCH = $(shell uname -m | sed 's/arm64/aarch64/')
ROOTDIR  = $(CURDIR)/../../../../..

LIBCUTILSDIR = $(ROOTDIR)/external/android/system/core/libcutils
LIBUTILSDIR  = $(ROOTDIR)/external/android/system/core/libutils
LIBBASEDIR   = $(ROOTDIR)/external/android/system/libbase
LIBHWBINDER  = $(ROOTDIR)/external/android/system/libhwbinder
LIBLOGDIR    = $(ROOTDIR)/external/android/system/logging
LIBJSONCPP   = $(ROOTDIR)/external/jsoncpp/jsoncpp

# Please install these tools before building.
# sudo apt install bison flex clang libc++-dev libc++abi-dev
# In Ubuntu 20.04.6 LTS, the tools' version:
# bison=3.5.1
# clang=10.0.0-4ubuntu1
# flex=2.6.4
# You can use these tools on macOS too.
BISON = bison
CC = clang++
ifeq ($(SYSTEM), darwin)
CC += -arch $(shell uname -m)
endif
FLEX = flex

CXXFLAGS += -stdlib=libc++
CXXFLAGS += -std=gnu++17
CXXFLAGS += -Wall -Wextra
CXXFLAGS += -O2
CXXFLAGS += -pthread

LDFLAGS = -static-libstdc++ -lc++abi -lssl -lcrypto

CXXFLAGS += -I./
CXXFLAGS += -Ihashing/include
CXXFLAGS += -Ihidl
CXXFLAGS += -Ihost_utils/include
CXXFLAGS += -Ihost_utils/include/hidl-util
CXXFLAGS += -Iutils/include/hidl-util
CXXFLAGS += -Iutils/include
CXXFLAGS += -I$(LIBBASEDIR)/include
CXXFLAGS += -I$(LIBCUTILSDIR)/include
CXXFLAGS += -I$(LIBHWBINDER)/include
CXXFLAGS += -I$(LIBJSONCPP)/include
CXXFLAGS += -I$(LIBLOGDIR)/include
CXXFLAGS += -I$(LIBUTILSDIR)/include

PROGNAME = $(ROOTDIR)/prebuilts/tools/hidl/$(SYSTEM)/$(SYS_ARCH)/hidl-gen

# main
CXXSRCS += main.cpp

# libhidl-gen
CXXSRCS += Annotation.cpp \
	ArrayType.cpp \
	CompoundType.cpp \
	ConstantExpression.cpp \
	DeathRecipientType.cpp \
	DocComment.cpp \
	EnumType.cpp \
	FmqType.cpp \
	HandleType.cpp \
	HidlTypeAssertion.cpp \
	Interface.cpp \
	Location.cpp \
	MemoryType.cpp \
	Method.cpp \
	NamedType.cpp \
	PointerType.cpp \
	ScalarType.cpp \
	Scope.cpp \
	StringType.cpp \
	Type.cpp \
	TypeDef.cpp \
	VectorType.cpp

# host_utils
CXXSRCS += host_utils/Formatter.cpp \
	host_utils/StringHelper.cpp

# utils
CXXSRCS += utils/FqInstance.cpp \
	utils/FQName.cpp

# ast
CXXSRCS += AST.cpp \
	Coordinator.cpp \
	generateCpp.cpp \
	generateCppImpl.cpp \
	generateDependencies.cpp \
	generateFormattedHidl.cpp \
	generateInheritanceHierarchy.cpp \
	generateJava.cpp \
	generateJavaImpl.cpp \
	generateVts.cpp \
	hidl-gen_l.cpp \
	hidl-gen_y.cpp

# hash
CXXSRCS += hashing/Hash.cpp

# libjsoncpp
CXXSRCS += $(LIBJSONCPP)/src/lib_json/json_writer.cpp \
	$(LIBJSONCPP)/src/lib_json/json_value.cpp

# libbase
CXXSRCS += $(LIBBASEDIR)/strings.cpp \
	$(LIBBASEDIR)/stringprintf.cpp \
	$(LIBBASEDIR)/logging.cpp \
	$(LIBBASEDIR)/file.cpp \
	$(LIBBASEDIR)/hex.cpp \
	$(LIBBASEDIR)/threads.cpp \
	$(LIBBASEDIR)/posix_strerror_r.cpp

# liblog
CXXSRCS += $(LIBLOGDIR)/log_event_list.cpp \
	$(LIBLOGDIR)/log_event_write.cpp \
	$(LIBLOGDIR)/logger_name.cpp \
	$(LIBLOGDIR)/logger_read.cpp \
	$(LIBLOGDIR)/logger_write.cpp \
	$(LIBLOGDIR)/logprint.cpp \
	$(LIBLOGDIR)/properties.cpp

OBJS = $(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(CXXSRCS)))

all: $(PROGNAME)

$(PROGNAME): $(OBJS)
	$(CC) $(CXXFLAGS) $(LDFLAGS) -o $(PROGNAME) $(OBJS)

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

%.o: %.cc
	$(CC) $(CXXFLAGS) -c $< -o $@

%.cpp %.h: %.yy
	@echo BISON $@
	$(BISON) -Lc++ --defines=$*.h -o $*.cpp $<

%.cpp: %.ll
	@echo FLEX $@
	$(FLEX) -o $@ $<

hidl-gen_l.cpp: hidl-gen_y.h

clean:
	rm $(OBJS) $(PROGNAME) hidl-gen_l.cpp hidl-gen_y.cpp hidl-gen_y.h *.hh
