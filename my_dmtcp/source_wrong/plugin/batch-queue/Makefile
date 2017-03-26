PLUGIN=batch-queue
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
top_builddir = ../..
top_srcdir = ../..
srcdir = .

targetdir = $(top_builddir)

ifeq ($(M32),1)
  targetdir = $(top_builddir)/lib/$(PACKAGE)/32
endif

LIBNAME=$(targetdir)/lib/${PACKAGE}/libdmtcp_${PLUGIN}.so

DMTCP_INCLUDE_PATH = $(top_srcdir)/include
JALIB_PATH = $(top_srcdir)/jalib

INCLUDES = -I$(JALIB_PATH) -I$(DMTCP_INCLUDE_PATH) -I$(top_builddir)/include
CFLAGS += -DHAVE_CONFIG_H -fPIC -g
CXXFLAGS += -DHAVE_CONFIG_H -fPIC -g
ifeq ($(M32),1)
CFLAGS += -m32 -Wa,--32
CXXFLAGS += -m32 -Wa,--32
LDFLAGS += -m32 -Wl,-m32 -Wl,-melf_i386 -Wa,--32
endif

COMPILE = $(CC) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) -c -o $@
LINK = $(CC) $(CFLAGS) $(LDFLAGS) -o $@
CXXCOMPILE = $(CXX) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c -o $@
CXXLINK = $(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@

HEADERS = $(DMTCP_INCLUDE_PATH)/util.h $(DMTCP_INCLUDE_PATH)/dmtcp.h \
	  $(DMTCP_INCLUDE_PATH)/procmapsarea.h \
	  $(JALIB_PATH)/jassert.h $(JALIB_PATH)/jalloc.h \
	  $(JALIB_PATH)/jconvert.h $(JALIB_PATH)/jfilesystem.h \
	  $(JALIB_PATH)/jalib.h rm_main.h rm_torque.h rm_slurm.h rm_utils.h rm_pmi.h

OBJS = rm_main.o rm_torque.o rm_slurm.o rm_utils.o rm_pmi.o
SOURCES = rm_main.cpp rm_torque.cpp rm_slurm.cpp rm_utils.cpp rm_pmi.cpp

DISCOVER_OBJS = discover_dmtcpinput.o discover_resources.o discover_slurm.o \
		discover_torque.o
DISCOVER_HEADERS = discover_dmtcpinput.h discover_resources.h discover_slurm.h \
		   discover_torque.h


LIB_LDFLAGS = -shared -Wl,--unresolved-symbols=ignore-all

LDADD = -ldl -lpthread

all: default

default: build

ifeq ($(M32),0)
  RM_BINS = $(targetdir)/bin/dmtcp_discover_rm \
	    $(targetdir)/bin/dmtcp_rm_loclaunch
endif

build: ${LIBNAME} $(RM_BINS)

tidy:
	rm -rf dmtcp-autotest-* ckpt_*_files
	rm -f ckpt_*.dmtcp dmtcp_restart_script* core*

clean: tidy
	-rm -f *.o
	-rm -f $(LIBNAME)
	-rm -f $(targetdir)/bin/dmtcp_discover_rm

distclean: clean
	rm -f Makefile

install-libs: all
	${INSTALL_PROGRAM} ${LIBNAME} $(DESTDIR)$(pkglibdir)

uninstall-libs:
	rm -f $(DESTDIR)$(pkglibdir)/$(LIBNAME)

install: install-libs
	${INSTALL_PROGRAM} $(targetdir)/bin/dmtcp_discover_rm $(DESTDIR)$(bindir)
	${INSTALL_SCRIPT} $(targetdir)/bin/dmtcp_rm_loclaunch $(DESTDIR)$(bindir)

uninstall: uninstall-libs
	rm -f $(DESTDIR)$(bindir)/dmtcp_discover_rm
	rm -f $(DESTDIR)$(bindir)/dmtcp_rm_loclaunch


${LIBNAME}: $(OBJS) ${HEADERS}
	@rm -f ${LIBNAME}
	$(CXXLINK) $(OBJS) $(LIB_LDFLAGS) $(LDADD)

$(targetdir)/bin/dmtcp_rm_loclaunch:
	cp $(srcdir)/dmtcp_rm_loclaunch.sh $(targetdir)/bin/dmtcp_rm_loclaunch

$(targetdir)/bin/dmtcp_discover_rm: dmtcp_discover_rm.cpp $(DISCOVER_OBJS) \
  $(DISCOVER_HEADERS)
	$(CXXLINK) $(INCLUDES) $(CPPFLAGS) $(DISCOVER_OBJS) -o $@ $<

%.o: %.c ${HEADERS}
	$(COMPILE)  $<

%.o: %.cpp ${HEADERS}
	$(CXXCOMPILE)  $<

discover_%.o: discover_%.c ${DISCOVER_HEADERS}
	$(COMPILE)  $<

discover_%.o: discover_%.cpp ${DISCOVER_HEADERS}
	$(CXXCOMPILE)  $<

.PHONY: default all build tidy clean distclean install uninstall
