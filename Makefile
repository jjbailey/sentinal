# Makefile
#
# Copyright (c) 2021-2025 jjb
# All rights reserved.
#
# This source code is licensed under the MIT license found
# in the root directory of this source tree.

WARNINGS := -Wno-unused-result -Wunused-variable -Wunused-but-set-variable

CC := gcc
CFLAGS := -O2 -fstack-protector-strong -pthread $(WARNINGS)

# CC := clang-19
# CFLAGS := -O -fstack-protector-strong -pthread $(WARNINGS)

# CC := zig cc
# CFLAGS := -O -fstack-protector-strong -pthread $(WARNINGS)

LDFLAGS :=
PCRELIB := -lpcre2-8
LIBS := -lpthread $(PCRELIB) -lm -lsqlite3

SEN_HOME := /opt/sentinal
SEN_BIN := $(SEN_HOME)/bin
SEN_ETC := $(SEN_HOME)/etc
SEN_DOC := $(SEN_HOME)/doc
PCRE_DIR := /usr/lib/sqlite3

SENOBJS := sentinal.o convexpire.o dfsthread.o expthread.o findfile.o \
	findmnt.o fullpath.o iniget.o ini.o logname.o logretention.o \
	logsize.o namematch.o outputs.o pcrecompile.o postcmd.o \
	readini.o rlimit.o rmfile.o signals.o slmthread.o sql.o \
	strdel.o strlcat.o strlcpy.o strreplace.o threadname.o \
	threadtype.o validdbname.o verifyids.o workcmd.o workthread.o

SPMOBJS := sentinalpipe.o fullpath.o iniget.o ini.o strlcpy.o validdbname.o
PCFOBJS := pcrefind.o fullpath.o pcrecompile.o pcrematch.o strlcpy.o
PCTOBJS := pcretest.o pcrecompile.o pcrematch.o
PCRESO := pcre2.so

# Default target
all: sentinal sentinalpipe dfree pcrefind pcretest $(PCRESO)

sentinal: $(SENOBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
sentinalpipe: $(SPMOBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
dfree: dfree.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
pcrefind: $(PCFOBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
pcretest: $(PCTOBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Pattern rule for .o: avoids repeating for every source file
%.o: %.c
	$(CC) $(CFLAGS) -c $<

$(PCRESO): pcre2.c
	$(CC) -shared -o $@ -fPIC -W -Werror $^ $(PCRELIB)

# Header dependencies (can be replaced by auto dependencies)
$(SENOBJS) $(SPMOBJS) $(PCTOBJS): sentinal.h basename.h ini.h

install: all
	mkdir -p $(SEN_HOME) $(SEN_BIN) $(SEN_ETC) $(SEN_DOC) $(PCRE_DIR)
	chown root:root $(SEN_HOME) $(SEN_BIN) $(SEN_ETC) $(SEN_DOC)
	install -o root -g root -m 755 sentinal sentinalpipe dfree pcrefind pcretest -t $(SEN_BIN)
	install -o root -g root -m 644 examples/example1.ini -T $(SEN_ETC)/example1.ini
	install -o root -g root -m 644 examples/example1-split.ini -T $(SEN_ETC)/example1-split.ini
	install -o root -g root -m 644 examples/example2.ini -T $(SEN_ETC)/example2.ini
    #install -o root -g root -m 755 pcre2.so $(PCRE_DIR)/pcre2.so
	cp -p README.md README.d/README.* $(SEN_DOC)
	chown -R root:root $(SEN_DOC)

systemd:
	sed "s,INI_FILE,$(SEN_ETC)," services/sentinal.systemd > sentinal.service
	install -o root -g root -m 644 sentinal.service -t /etc/systemd/system
	sed "s,INI_FILE,$(SEN_ETC)," services/sentinalpipe.systemd > sentinalpipe.service
	install -o root -g root -m 644 sentinalpipe.service -t /etc/systemd/system

deb:
	bash packaging/debian/debian.sh

rpm:
	bash packaging/redhat/redhat.sh

clean:
	$(RM) *.o $(PCRESO) sentinal sentinalpipe dfree pcrefind pcretest
	$(RM) sentinal.service sentinalpipe.service
	$(RM) -fr packaging/debian/sentinal_* packaging/redhat/sentinal-* packaging/redhat/rpmbuild

.PHONY: all install clean systemd deb rpm

# vim: set tabstop=4 shiftwidth=4 noexpandtab:
