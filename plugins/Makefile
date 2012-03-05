prefix=/usr/local

plugindir=$(prefix)/lib/librtmp/plugins
PLUGINDIR=$(DESTDIR)$(plugindir)

PLUGINS=example

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
AR=$(CROSS_COMPILE)ar

SYS=posix

SOX_posix=so
SOX_darwin=dylib
SOX_mingw=so	# useless
SOX=$(SOX_$(SYS))
SO_posix=so.0
SO_darwin=0.so
SO_mingw=dll
SO_EXT=$(SO_$(SYS))

SO_LDFLAGS_posix=-shared -Wl,-soname,$@
SO_LDFLAGS_darwin=-dynamiclib -twolevel_namespace -undefined dynamic_lookup \
	-fno-common -headerpad_max_install_names -install_name $(plugindir)/$@
SO_LDFLAGS_mingw=-shared -Wl,--out-implib,$@.a
SO_LDFLAGS=$(SO_LDFLAGS_$(SYS))

INSTALL_IMPLIB_posix=
INSTALL_IMPLIB_darwin=
INSTALL_IMPLIB_mingw=$(foreach pluginname,$(PLUGINS),cp $(pluginname).dll $(PLUGINDIR);)
INSTALL_IMPLIB=$(INSTALL_IMPLIB_$(SYS))

RTMP_PLUGIN_API=$(shell grep "SO_VERSION *=" ../librtmp/Makefile  | cut -d = -f 2)

OPT=-O2
CFLAGS=-Wall -fPIC -I.. $(XCFLAGS) $(OPT) -DRTMP_PLUGIN_API=$(RTMP_PLUGIN_API)
LDFLAGS=$(XLDFLAGS)

PLUGIN_SO_FILES=$(foreach pluginname,$(PLUGINS), $(pluginname).$(SO_EXT))

all: $(PLUGIN_SO_FILES)

example.o: example.c example.h Makefile

example.$(SO_EXT): example.o
	$(CC) $(SO_LDFLAGS) $(LDFLAGS) -o $@ $^ $> -L../librtmp -lrtmp

install: $(PLUGIN_SO_FILES)
	mkdir -p $(PLUGINDIR)
	cp $(PLUGIN_SO_FILES) $(PLUGINDIR)
	$(INSTALL_IMPLIB)
	cd $(PLUGINDIR); $(foreach pluginname,$(PLUGINS),ln -sf $(pluginname).$(SO_EXT) $(pluginname).$(SOX);)

clean:
	rm -f *.o *.a *.$(SOX) *.$(SO_EXT)
