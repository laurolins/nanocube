bin_PROGRAMS = stree_serve stree_dmp

# AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -D_GLIBCXX_USE_NANOSLEEP

VERSION = $(shell cat $(top_srcdir)/VERSION)
AM_CXXFLAGS += -DVERSION=\"$(VERSION)\"

stree_dmp_SOURCES  = stree_dmp.cc  Stopwatch.cc Stopwatch.hh

stree_serve_LDFLAGS = -ldl -lpthread
stree_serve_SOURCES =      \
stree_serve.cc             \
STree.cc                   \
mongoose.c                 \
Logger.cc		   \
Util.cc                    \
Server.cc                  \
TimeBinFunction.cc         \
MercatorProjection.cc      \
MemoryUtil.cc              \
Stopwatch.cc               \
QuadTree.cc                \
QuadTreeNode.cc            \
TimeSeries.cc              \
ContentHolder.hh           \
FlatTree.hh                \
MemoryUtil.hh              \
MercatorProjection.hh      \
QuadTree.hh                \
QuadTreeNode.hh            \
QuadTreeUtil.hh            \
Server.hh                  \
STree.hh                   \
TimeBinFunction.hh         \
Stopwatch.hh               \
TaggedPointer.hh           \
TimeSeries.hh              \
mongoose.h                 \
small_vector.hh


# STreeSerialization.hh      \
# TimeSeriesSerialization.hh \
# QuadTreeSerialization.hh   \
# FlatTreeSerialization.hh   \
# QuadTreeSerialization.cc 


# lib_LTLIBRARIES = libdatatiles.la
# libdatatiles_la_SOURCES = \
# MemoryUtil.cc             \
# Stopwatch.cc              \
# MercatorProjection.cc     \
# Util.cc                   \
# QuadTreeSerialization.cc  \
# QuadTreeNode.cc           \
# STree.cc                  \
# QuadTree.cc               \
# TimeSeries.cc
# libdatatiles_la_CPPFLAGS = -I$(top_srcdir)/src
# libdatatiles_la_includedir = $(includedir)/datatiles
# libdatatiles_la_include_HEADERS = \
# ContentHolder.hh                  \
# Util.hh                           \
# FlatTree.hh                       \
# FlatTreeSerialization.hh          \
# MemoryUtil.hh                     \
# MercatorProjection.hh             \
# QuadTree.hh                       \
# QuadTreeNode.hh                   \
# QuadTreeSerialization.hh          \
# QuadTreeUtil.hh                   \
# STree.hh                          \
# STreeSerialization.hh             \
# Stopwatch.hh                      \
# TaggedPointer.hh                  \
# TimeSeries.hh                     \
# Tuple.hh                          \
# TimeSeriesSerialization.hh        \
# small_vector.hh
