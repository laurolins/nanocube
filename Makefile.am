AUTOMAKE_OPTIONS=subdir-objects
ACLOCAL_AMFLAGS=-I m4


if WITH_POLYCOVER
lib_LIBRARIES=libpolycover.a
libpolycover_a_SOURCES=\
	src/polycover/area.cc \
	src/polycover/area.hh \
	src/polycover/boundary.cc \
	src/polycover/boundary.hh \
	src/polycover/cells.cc \
	src/polycover/cells.hh \
	src/polycover/geometry.cc \
	src/polycover/geometry.hh \
	src/polycover/glue.cc \
	src/polycover/infix_iterator.hh \
	src/polycover/labeled_tree.cc \
	src/polycover/labeled_tree.hh \
	src/polycover/maps.cc \
	src/polycover/maps.hh \
	src/polycover/mipmap.cc \
	src/polycover/mipmap.hh \
	src/polycover/polycover.cc \
	src/polycover/polycover.h \
	src/polycover/polycover.hh \
	src/polycover/signal.hh \
	src/polycover/tessellation.cc \
	src/polycover/tessellation.hh \
	src/polycover/tokenizer.cc \
	src/polycover/tokenizer.hh \
	src/polycover/glu/dict.c \
	src/polycover/glu/memalloc.c \
	src/polycover/glu/sweep.c \
	src/polycover/glu/tessmono.c \
	src/polycover/glu/geom.c \
	src/polycover/glu/mesh.c \
	src/polycover/glu/priorityq.c \
	src/polycover/glu/tess.c \
	src/polycover/glu/normal.c \
	src/polycover/glu/render.c \
	src/polycover/glu/tessellate.c
endif

# lib_LTLIBRARIES+=libnanocube_app.la
# # the tentative platform independent part
# libnanocube_app_la_SOURCES=src/app.c
# libnanocube_app_la_CFLAGS=
# libnanocube_app_la_LDFLAGS=-module -avoid-version -shared
# libnanocube_app_la_LIBADD=-lm
# if LINUX
# libnanocube_app_la_CFLAGS+=-std=gnu11 -DOS_LINUX -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-strict-aliasing -Wno-unused-function -Wno-unused-variable -Wno-implicit-function-declaration
# endif
# if OSX
# libnanocube_app_la_CFLAGS+=-std=c11 -DOS_MAC -Wno-unused-variable -Wno-null-dereference -Wno-unused-function
# endif
# if WITH_POLYCOVER
# endif

bin_PROGRAMS=nanocube
nanocube_SOURCES=src/platform_dependent/nix_app.c
nanocube_CFLAGS=-pthread -O2
nanocube_LDADD=-lm
if WITH_POLYCOVER
nanocube_CFLAGS+=-DPOLYCOVER
nanocube_LDADD+=libpolycover.a
nanocube_LDADD+=-lstdc++
endif
if LINUX
nanocube_CFLAGS+=-std=gnu11 -DOS_LINUX -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-strict-aliasing -Wno-unused-function -Wno-unused-variable -Wno-implicit-function-declaration
endif
if OSX
nanocube_CFLAGS+=-std=c11 -DOS_MAC -Wno-unused-variable -Wno-null-dereference -Wno-unused-function
endif

SUBDIRS=web

