
LN_S=ln -s
OPENMP_CFLAGS=-fopenmp
OPENMP_CXXFLAGS=-fopenmp
CC = gcc
M32=0
CFLAGS = -g -O0 -Wall
CPPFLAGS = 
CXX = g++
CXXFLAGS = -g -O0 -Wall
HAS_JAVA= yes
HAS_JAVAC= yes
LDFLAGS = 
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
top_builddir = ..
top_srcdir = ..
srcdir = .

targetdir = $(top_builddir)

DMTCP_INCLUDE=${top_srcdir}/include

CFLAGS += -I${DMTCP_INCLUDE} -DDMTCP -fPIC --std=gnu99 -g
CXXFLAGS += -I${DMTCP_INCLUDE} -DDMTCP -fPIC -g


ifeq ($(M32),1)
  targetdir = $(top_builddir)/lib/$(PACKAGE)/32
  CFLAGS += -m32 -Wa,--32
  CXXFLAGS += -m32 -Wa,--32
  LDFLAGS += -m32 -Wl,-m32 -Wl,-melf_i386 -Wa,--32
else
  HAS_READLINE=no
  READLINE_LIBS=-lreadline -lhistory -lcurses
  HAS_MPICH=no
  MPICH_MPICC=
  HAS_OPENMPI=yes
  OPENMPI_MPICC=/usr/lib64/openmpi/bin/mpicc
  HAS_CILK=no
  CILKC=no
endif

PWD=$(shell pwd)

TESTS=${notdir ${basename ${shell ls $(srcdir)/*.c $(srcdir)/*.cpp $(srcdir)/*.cilk | grep -v 'hellompi.c'}}} \
	${addsuffix .class,${notdir ${basename ${shell ls $(srcdir)/java*.java}}}} \
	hellompich openmpi

tests: $(TESTS)
	cd plugin && ${MAKE}
	#${MAKE} -C credentials

tidy:
	rm -f ckpt_*.dmtcp dmtcp_restart_script* \
	  dmtcp-shared-memory.* dmtcp-test-typescript.tmp core*
	rm -rf ckpt_*
	cd plugin && $(MAKE) tidy > /dev/null

clean: tidy
	rm -f $(TESTS) *.pyc *.so
	#${MAKE} -C credentials clean
	cd plugin && $(MAKE) clean

distclean: clean
	cd plugin && $(MAKE) distclean
	#${MAKE} -C credentials distclean
	rm -f Makefile

readline: readline.c
ifeq ($(HAS_READLINE),yes)
	$(CC) -o $@ $< $(CFLAGS) $(READLINE_LIBS)
else
	#$@: required libraries missing, skipping test
endif

timer: timer.c
	$(CC) -o $@ $< $(CFLAGS) -lrt

clock: clock.c
	$(CC) -o $@ $< $(CFLAGS) -lrt

# Assume OpenMP exists only if OPENMP_CFLAGS is set.
openmp-%: openmp-%.c
ifneq (${OPENMP_CFLAGS},)
	-${CC} ${CFLAGS} ${OPENMP_CFLAGS} -o $@ $<
else
	#$@: This ${CC} does not support OpenMP, skipping test
endif

# Assume OpenMP exists for C++ only if OPENMP_CXXFLAGS is set.
openmp-%: openmp-%.cpp
ifneq (${OPENMP_CXXFLAGS},)
	-${CXX} ${CXXFLAGS} ${OPENMP_CXXFLAGS} -o $@ $<
else
	#$@: This ${CXX} does not support OpenMP, skipping test
endif

hellompich: hellompi.c
ifeq ($(HAS_MPICH),yes)
	- $(MPICH_MPICC) -o $@ $< $(CFLAGS)
else
	#$@: required libraries missing, skipping test
endif

openmpi: hellompi.c
ifeq ($(HAS_OPENMPI),yes)
	- $(OPENMPI_MPICC) -o $@ $< $(CFLAGS)
else
	#$@: required libraries missing, skipping test
endif

DMTCPAWARE_EXE=$(patsubst %.c,%,$(wildcard dmtcpaware*.c))
DMTCPPLUGIN_HEADER=$(DMTCP_INCLUDE)/dmtcp.h

${DMTCPAWARE_EXE}: dmtcpaware%: dmtcpaware%.c $(DMTCPPLUGIN_HEADER)
	-$(CC) -o $@ -I$(DMTCP_INCLUDE)/dmtcp/include $< $(CFLAGS)

dmtcp3: dmtcp3.c
	-$(CC) -o $@ $< $(CFLAGS) -lpthread

dmtcp4: dmtcp4.c
	-$(CC) -o $@ $< $(CFLAGS) -lpthread

dmtcp5: dmtcp5.c
	-$(CC) -o $@ $< $(CFLAGS) -lpthread

pthread%: pthread%.c
	-$(CC) -o $@ $< $(CFLAGS) -lpthread

posix-mq%: posix-mq%.c
	-$(CC) -o $@ $< $(CFLAGS) -lrt

inotify%: inotify%.c
	-$(CC) -o $@ $< $(CFLAGS) -lpthread

#ignore printf format errors
syscall-tester: syscall-tester.c
	-$(CC) -o $@ $< $(CFLAGS) -Wno-format

# dlopen will dlopen/dlclose libdlopen-lib[12].so
libdlopen-lib1.so: dlopen1.c
	${CC} ${CFLAGS} -shared -fPIC  -DLIB1 -o libdlopen-lib1.so $<
libdlopen-lib2.so: dlopen1.c
	${CC} ${CFLAGS} -shared -fPIC  -DLIB2 -o libdlopen-lib2.so $<
dlopen%: dlopen%.c libdlopen-lib1.so libdlopen-lib2.so
	${CC} $(CFLAGS) -o $@ $< -ldl

libdlopen-lib3.so: dlopen2.cpp
	${CXX} ${CXXFLAGS} -shared -fPIC -DLIB3 -o libdlopen-lib3.so $<
libdlopen-lib4.so: dlopen2.cpp
	${CXX} ${CXXFLAGS} -shared -fPIC -DLIB4 -o libdlopen-lib4.so $<
dlopen%: dlopen%.cpp libdlopen-lib4.so libdlopen-lib4.so
	${CXX} ${CXXFLAGS} -o $@ $< -ldl

%.class: %.java
ifeq ($(HAS_JAVAC),yes)
	javac $< -d .
else
	#$@: required 'javac' missing, skipping test
endif

cilk%: cilk%.cilk
ifeq ($(HAS_CILK),yes)
	${CILKC} -o $@ $<
else
	#$@: required 'cilkc' missing, skipping test
endif

%: %.c
	$(CC) -o $@ $< $(CFLAGS)

%: %.cpp
	$(CXX) -o $@ $< $(CXXFLAGS)
