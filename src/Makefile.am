AUTOMAKE_OPTIONS=subdir-objects
ACLOCAL_AMFLAGS=-I m4

lib_LTLIBRARIES=

if WITH_POLYCOVER
lib_LTLIBRARIES+=libpolycover.la
libpolycover_la_SOURCES=\
	polycover/area.cc \
	polycover/area.hh \
	polycover/boundary.cc \
	polycover/boundary.hh \
	polycover/cells.cc \
	polycover/cells.hh \
	polycover/geometry.cc \
	polycover/geometry.hh \
	polycover/glue.cc \
	polycover/infix_iterator.hh \
	polycover/labeled_tree.cc \
	polycover/labeled_tree.hh \
	polycover/maps.cc \
	polycover/maps.hh \
	polycover/mipmap.cc \
	polycover/mipmap.hh \
	polycover/polycover.cc \
	polycover/polycover.h \
	polycover/polycover.hh \
	polycover/signal.hh \
	polycover/tessellation.cc \
	polycover/tessellation.hh \
	polycover/tokenizer.cc \
	polycover/tokenizer.hh \
	polycover/glu/dict.c \
	polycover/glu/memalloc.c \
	polycover/glu/sweep.c \
	polycover/glu/tessmono.c \
	polycover/glu/geom.c \
	polycover/glu/mesh.c \
	polycover/glu/priorityq.c \
	polycover/glu/tess.c \
	polycover/glu/normal.c \
	polycover/glu/render.c \
	polycover/glu/tessellate.c
endif

lib_LTLIBRARIES+=libnanocube_app.la
bin_PROGRAMS=nanocube

# the tentative platform independent part
libnanocube_app_la_SOURCES=app.c
libnanocube_app_la_CFLAGS=
libnanocube_app_la_LDFLAGS=-module -avoid-version -shared
libnanocube_app_la_LIBADD=-lm
if LINUX
libnanocube_app_la_CFLAGS+=-std=gnu11 -DOS_LINUX -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-strict-aliasing -Wno-unused-function -Wno-unused-variable -Wno-implicit-function-declaration
endif
if OSX
libnanocube_app_la_CFLAGS+=-std=c11 -DOS_MAC -Wno-unused-variable -Wno-null-dereference -Wno-unused-function
endif
if WITH_POLYCOVER
libnanocube_app_la_CFLAGS+=-DPOLYCOVER
endif

nanocube_SOURCES=platform_dependent/nix_app.c
nanocube_CFLAGS=-pthread
nanocube_LDADD=-lm -ldl
if LINUX
nanocube_CFLAGS+=-std=gnu11 -DOS_LINUX -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-strict-aliasing -Wno-unused-function -Wno-unused-variable -Wno-implicit-function-declaration
endif
if OSX
nanocube_CFLAGS+=-std=c11 -DOS_MAC -Wno-unused-variable -Wno-null-dereference -Wno-unused-function
endif
if WITH_POLYCOVER
nanocube_CFLAGS+=-DPOLYCOVER
endif

