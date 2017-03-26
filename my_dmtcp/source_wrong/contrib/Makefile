
LN_S=ln -s -f
MKDIR_P=/bin/mkdir -p
CC=gcc
M32=0
INFINIBAND_SUPPORT=no
PACKAGE=dmtcp
PACKAGE_TARNAME=dmtcp-2.3.1
VERSION=2.3.1
prefix=/home/fss5/sawada/デスクトップ/takeover_sawada/my_dmtcp/prefix
exec_prefix=${prefix}
datarootdir=${prefix}/share
bindir=${exec_prefix}/bin
libdir=${exec_prefix}/lib
docdir=${datarootdir}/doc/${PACKAGE_TARNAME}
includedir=${prefix}/include
mandir=${datarootdir}/man
infodir=${datarootdir}/man

CFLAGS += -DHAVE_CONFIG_H -fPIC -g
CXXFLAGS += -DHAVE_CONFIG_H -fPIC -g
ifeq ($(M32),1)
CFLAGS += -m32 -Wa,--32
CXXFLAGS += -m32 -Wa,--32
LDFLAGS += -m32 -Wl,-m32 -Wl,-melf_i386 -Wa,--32
endif

plugins = 
plugins_distclean = ckptfile infiniband

ifeq ($(INFINIBAND_SUPPORT),yes)
  plugins += infiniband  # ib2tcp
endif

default: build

build: $(plugins)

all: default

ckptfile:
	cd $@ && $(MAKE)

$(plugins):
	cd $@ && $(MAKE)

tidy:
	rm -rf dmtcp-autotest-* ckpt_*_files
	rm -f ckpt_*.dmtcp dmtcp_restart_script* \
	  dmtcp-shared-memory.* dmtcp-test-typescript.tmp core*
	rm -rf ckpt_*

clean: tidy
	for p in $(plugins); do (cd $$p && $(MAKE) clean); done

distclean: clean
	for p in $(plugins); do \
	  (cd $$p && $(MAKE) distclean 2>/dev/null); \
	done
	for p in $(plugins_distclean); do \
	  (cd $$p && $(MAKE) distclean 2>/dev/null); \
	done
	rm -f Makefile

install-libs: all
	test -e $(DESTDIR)$(libdir) || $(MKDIR_P) $(DESTDIR)$(libdir)
	for p in $(plugins); do (cd $$p && $(MAKE) install-libs); done

install: all
	test -e $(DESTDIR)$(bindir) || $(MKDIR_P) $(DESTDIR)$(bindir)
	test -e $(DESTDIR)$(libdir) || $(MKDIR_P) $(DESTDIR)$(libdir)
	test -e $(DESTDIR)$(includedir) || $(MKDIR_P) $(DESTDIR)$(includedir)
	for p in $(plugins); do (cd $$p && $(MAKE) install); done

uninstall:
	for p in $(plugins); do (cd $$p && $(MAKE) uninstall); done

uninstall-libs:
	for p in $(plugins); do (cd $$p && $(MAKE) uninstall-libs); done

.PHONY: default all build infiniband ckptfile tidy clean distclean install uninstall
