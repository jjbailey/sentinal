# Makefile
#
# Copyright (c) 2021, 2022 jjb
# All rights reserved.
#
# This source code is licensed under the MIT license found
# in the root directory of this source tree.

all:		sentinal		\
			sentinalpipe	\
			pcrefind		\
			pcretest

SENOBJS=	sentinal.o		\
			convexpire.o	\
			dfsthread.o		\
			expthread.o		\
			findfile.o		\
			findmnt.o		\
			fullpath.o		\
			ini.o			\
			iniget.o		\
			logname.o		\
			logretention.o	\
			logsize.o		\
			mylogfile.o		\
			pcrecompile.o	\
			postcmd.o		\
			rlimit.o		\
			rmfile.o		\
			signals.o		\
			slmthread.o		\
			strlcat.o		\
			strlcpy.o		\
			strreplace.o	\
			threadcheck.o	\
			threadname.o	\
			verifyids.o		\
			workcmd.o		\
			workthread.o

SPMOBJS=	sentinalpipe.o	\
			fullpath.o		\
			ini.o			\
			iniget.o		\
			strlcpy.o

PCFOBJS=	pcrefind.o		\
			fullpath.o		\
			pcrecompile.o	\
			pcrematch.o		\
			strlcpy.o

PCTOBJS=	pcretest.o		\
			pcrecompile.o	\
			pcrematch.o

SEN_HOME=	/opt/sentinal
SEN_BIN=	$(SEN_HOME)/bin
SEN_ETC=	$(SEN_HOME)/etc
SEN_DOC=	$(SEN_HOME)/doc
SEN_TEST=	$(SEN_HOME)/tests

CC=			gcc
WARNINGS=	-Wno-unused-result -Wunused-variable -Wunused-but-set-variable
CFLAGS=		-g -fstack-protector -O2 -pthread $(WARNINGS)

PCRELIB=	-lpcre2-8

LIBS=		-lpthread $(PCRELIB) -lm

$(SENOBJS):	sentinal.h basename.h ini.h

$(SPMOBJS):	sentinal.h basename.h ini.h

$(PCTOBJS):	sentinal.h basename.h ini.h

sentinal:	$(SENOBJS)
			$(CC) $(LDFLAGS) -o $@ $(SENOBJS) $(LIBS)

sentinalpipe:	$(SPMOBJS)
				$(CC) $(LDFLAGS) -o $@ $(SPMOBJS) $(LIBS)

pcrefind:	$(PCFOBJS)
			$(CC) $(LDFLAGS) -o $@ $(PCFOBJS) $(LIBS)

pcretest:	$(PCTOBJS)
			$(CC) $(LDFLAGS) -o $@ $(PCTOBJS) $(LIBS)

install:	all
			mkdir -p $(SEN_HOME) $(SEN_BIN) $(SEN_ETC) $(SEN_DOC)
			chown root:root $(SEN_HOME) $(SEN_BIN) $(SEN_ETC) $(SEN_DOC)
			install -o root -g root -m 755 sentinal -t $(SEN_BIN)
			install -o root -g root -m 755 sentinalpipe -t $(SEN_BIN)
			install -o root -g root -m 755 pcretest -t $(SEN_BIN)
			install -o root -g root -m 644 tests/test4.ini -T $(SEN_ETC)/example.ini
			cp -p README.* $(SEN_DOC)
			chown -R root:root $(SEN_DOC)

tests:		all
			mkdir -p $(SEN_TEST)
			cp -p tests/test?.* $(SEN_TEST)
			cp -p tests/testmultilog.ini tests/testmultipipe.ini $(SEN_TEST)
			chown -R root:root $(SEN_TEST)

systemd:
			sed "s,INI_FILE,$(SEN_ETC),"	\
				services/sentinal.systemd > sentinal.service
			install -o root -g root -m 644 sentinal.service -t /etc/systemd/system
			sed "s,INI_FILE,$(SEN_ETC),"	\
				services/sentinalpipe.systemd > sentinalpipe.service
			install -o root -g root -m 644 sentinalpipe.service -t /etc/systemd/system

deb:
			bash packaging/debian/debian.sh

rpm:
			bash packaging/redhat/redhat.sh

clean:
			rm -f $(SENOBJS) $(SPMOBJS) $(PCFOBJS) $(PCTOBJS)
			rm -f sentinal sentinalpipe pcrefind pcretest
			rm -f sentinal.service sentinalpipe.service
			rm -fr packaging/debian/sentinal_*
			rm -fr packaging/redhat/sentinal-* packaging/redhat/rpmbuild

# vim: set tabstop=4 shiftwidth=4 noexpandtab:
