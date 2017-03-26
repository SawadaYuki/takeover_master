PLUGIN=ipc
CC = gcc
M32=0
CFLAGS = -g -O0 -Wall
CPPFLAGS = 
CXX = g++
CXXFLAGS = -g -O0 -Wall
LDFLAGS = 

# Allow the user to specify the install program.
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}

MKDIR_P = /bin/mkdir -p
PACKAGE = dmtcp
PACKAGE_BUGREPORT = dmtcp-forum@lists.sourceforge.net
PACKAGE_NAME = DMTCP
PACKAGE_STRING = DMTCP 2.3.1
PACKAGE_TARNAME = dmtcp
PACKAGE_URL = http://dmtcp.sourceforge.net
PACKAGE_VERSION = 2.3.1
prefix=/home/fss5/sawada/デスクトップ/takeover_sawada/my_dmtcp/prefix
exec_prefix=${prefix}
datarootdir=${prefix}/share
bindir = ${exec_prefix}/bin
docdir = ${datarootdir}/doc/${PACKAGE_TARNAME}
includedir = ${prefix}/include
infodir = ${datarootdir}/info
libdir = ${exec_prefix}/lib
pkglibdir = $(libdir)/dmtcp
top_builddir = ../../..
top_srcdir = ../../..
srcdir = .

targetdir = $(top_builddir)

ifeq ($(M32),1)
  targetdir = $(top_builddir)/lib/$(PACKAGE)/32
endif

LIBNAME=$(targetdir)/lib/${PACKAGE}/libdmtcp_${PLUGIN}.so

DMTCP_INCLUDE_PATH = $(top_srcdir)/include
JALIB_PATH = $(top_srcdir)/jalib

INCLUDES = -I$(JALIB_PATH) -I$(DMTCP_INCLUDE_PATH) \
	   -I$(top_builddir)/include -I$(srcdir)
CFLAGS += -DHAVE_CONFIG_H -fPIC -g
CXXFLAGS += -DHAVE_CONFIG_H -fPIC -g
ifeq ($(M32),1)
CFLAGS += -m32 -Wa,--32
CXXFLAGS += -m32 -Wa,--32
LDFLAGS += -m32 -Wl,-m32 -Wl,-melf_i386 -Wa,--32
endif

CXXCOMPILE = $(CXX) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c -o $@
CXXLINK = $(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@

COMMON_HEADERS = ipc.h util_ipc.h \
		 $(DMTCP_INCLUDE_PATH)/util.h $(DMTCP_INCLUDE_PATH)/dmtcp.h \
		 $(DMTCP_INCLUDE_PATH)/shareddata.h \
		 $(DMTCP_INCLUDE_PATH)/dmtcpalloc.h \
		 $(DMTCP_INCLUDE_PATH)/protectedfds.h \
		 $(DMTCP_INCLUDE_PATH)/virtualidtable.h \
		 $(DMTCP_INCLUDE_PATH)/protectedfds.h \
		 $(DMTCP_INCLUDE_PATH)/procmapsarea.h \
		 $(top_builddir)/include/config.h \
		 $(JALIB_PATH)/jassert.h $(JALIB_PATH)/jalloc.h \
		 $(JALIB_PATH)/jalib.h $(JALIB_PATH)/jbuffer.h \
		 $(JALIB_PATH)/jconvert.h $(JALIB_PATH)/jfilesystem.h \
		 $(JALIB_PATH)/jserialize.h $(JALIB_PATH)/jsocket.h

CONNECTION_HEADERS = connection.h connectionidentifier.h connectionlist.h
SOCKET_HEADERS = socket/connectionrewirer.h socket/kernelbufferdrainer.h \
		 socket/socketwrappers.h socket/socketconnection.h \
		 socket/socketconnlist.h
FILE_HEADERS = file/filewrappers.h file/fileconnection.h file/fileconnlist.h
EVENT_HEADERS = event/eventwrappers.h event/eventconnection.h \
		event/eventconnlist.h event/util_descriptor.h

SYSV_HEADERS = sysv/sysvipc.h sysv/sysvipcwrappers.h

TIMER_HEADERS = timer/timerwrappers.h timer/timerlist.h

SSH_HEADERS = ssh/ssh.h ssh/sshdrainer.h

OBJS = ipc.o util_ipc.o connection.o connectionidentifier.o connectionlist.o \
       connectionrewirer.o kernelbufferdrainer.o \
       socketwrappers.o socketconnection.o socketconnlist.o \
       util_descriptor.o eventwrappers.o eventconnection.o eventconnlist.o\
       filewrappers.o fileconnection.o fileconnlist.o posixipcwrappers.o \
       sysvipc.o sysvipcwrappers.o \
       ssh.o util_ssh.o sshdrainer.o\
       timerlist.o timerwrappers.o

LIB_LDFLAGS = -shared -Wl,--unresolved-symbols=ignore-all

LDADD = -ldl -lpthread -lrt

all: default

default: build


ifeq ($(M32),0)
  DMTCP_SSH_BINS = $(targetdir)/bin/dmtcp_ssh $(targetdir)/bin/dmtcp_sshd
endif

build: ${LIBNAME} $(DMTCP_SSH_BINS)

tidy:
	rm -rf dmtcp-autotest-* ckpt_*_files
	rm -f ckpt_*.dmtcp dmtcp_restart_script* core*

clean: tidy
	-rm -f *.o
	-rm -f $(LIBNAME)
	-rm -f $(targetdir)/bin/dmtcp_ssh*

distclean: clean
	rm -f Makefile

install-libs: all
	${INSTALL_PROGRAM} ${LIBNAME} $(DESTDIR)$(pkglibdir)

uninstall-libs:
	rm -f $(DESTDIR)$(pkglibdir)/$(LIBNAME)

install: install-libs
	${INSTALL_PROGRAM} $(targetdir)/bin/dmtcp_ssh* $(DESTDIR)$(bindir)

uninstall: uninstall-libs
	rm -f $(DESTDIR)$(bindir)/dmtcp_ssh*

$(targetdir)/bin/%: ssh/%.cpp ${COMMON_HEADERS} util_ssh.o
	$(CXXLINK) $(INCLUDES) $(CPPFLAGS) util_ssh.o util_ipc.o $<

${LIBNAME}: $(OBJS)
	@rm -f ${LIBNAME}
	$(CXXLINK) $(OBJS) $(LIB_LDFLAGS) $(LDADD)

%.o: %.cpp ${COMMON_HEADERS} ${CONNECTION_HEADERS}
	$(CXXCOMPILE)  $<

%.o: ssh/%.cpp ${COMMON_HEADERS}
	$(CXXCOMPILE)  $<

%.o: socket/%.cpp ${COMMON_HEADERS} ${CONNECTION_HEADERS} ${SOCKET_HEADERS}
	$(CXXCOMPILE)  $<

%.o: file/%.cpp ${COMMON_HEADERS} ${CONNECTION_HEADERS} ${FILE_HEADERS}
	$(CXXCOMPILE)  $<

%.o: event/%.cpp ${COMMON_HEADERS} ${CONNECTION_HEADERS} ${EVENT_HEADERS}
	$(CXXCOMPILE)  $<

%.o: sysv/%.cpp ${COMMON_HEADERS} ${SYSV_HEADERS}
	$(CXXCOMPILE)  $<

%.o: timer/%.cpp ${COMMON_HEADERS} ${TIMER_HEADERS}
	$(CXXCOMPILE)  $<

.PHONY: default all build tidy clean distclean install uninstall
