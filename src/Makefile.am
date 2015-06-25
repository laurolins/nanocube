# EXTRA_DATA = extra
# extradir = $(prefix)/bin
# extra_DATA = nanocube-dmpvu nanocube-benchmark nanocube-monitor

# dist_bin_SCRIPTS = nanocube-dmpvu nanocube-benchmark nanocube-monitor nanocube-csv

#
# ncbuild is not robust enough
#
# install-exec-hook:
# 	cp $(top_srcdir)/src/ncbuild $(prefix)/bin
# 	chmod +x $(prefix)/bin/ncbuild

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS =              \
nanocube-binning-dmp        \
nanocube-leaf               \
nc_q25_u2_u4                \
nc_q25_c1_u2_u8             \
nc_q25_c1_u4_u8             \
nc_q25_c1_u2_u4             \
nc_q25_c4_u2_u4             \
nc_q25_c1_c1_u2_u4          \
nc_q25_c1_c1_c1_u2_u4       \
nc_q25_c1_c1_c1_c1_u2_u4    \
nc_q25_c1_c1_c1_c1_c1_u2_u4 \
nc_q20_q20_u4_u4            \
nc_q25_c2_u2_u4


#
# nc_q25_u2_u4          \
# nc_q20_q20_u2_u4      \
# nc_q25_q25_u2_u4      \
# nc_q25_c1_c1_u2_u4    \
# nc_q25_c1_c1_c1_u2_u4 \
# nc_q25_c1_u2_u8       \
# nc_q17_c1_c1_c1_u2_u4 \
# nc_q25_c1_c2_u2_u4    \
#

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/mongoose
AM_CXXFLAGS = -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD $(BOOST_CPPFLAGS) -DHAVE_SSTREAM
AM_LDFLAGS = $(BOOST_THREAD_LDFLAGS) $(BOOST_SYSTEM_LIBS) $(BOOST_ASIO_LDFLAGS)

#AM_CPPFLAGS = -I$(top_srcdir)/src
#AM_CXXFLAGS = -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD $(BOOST_CPPFLAGS)
#AM_LDFLAGS = $(BOOST_THREAD_LDFLAGS)
#AM_LDFLAGS += $(BOOST_MPI_LDFLAGS)

LIBS += $(BOOST_THREAD_LIBS)
LIBS += $(BOOST_SYSTEM_LIBS)
LIBS += $(BOOST_ASIO_LIBS)

#LIBS += $(BOOST_MPI_LIBS)
#LIBS += $(BOOST_SERIALIZATION_LIBS)

# nanocube_SOURCES =    \
# nanocube.cc

nanocube_leaf_SOURCES = \
nanocube-leaf.cc        \
DumpFile.cc

nanocube_binning_dmp_SOURCES = \
ncdmp.cc              \
ncdmp_base.cc         \
ncdmp_base.hh         \
TimeBinFunction.cc    \
TimeBinFunction.hh    \
DumpFile.cc           \
DumpFile.hh           \
MercatorProjection.cc \
MercatorProjection.hh

# nanocube_distribute_SOURCES = \
# nanocube-distribute.cc        \
# NanoCubeSchema.cc             \
# NanoCubeSchema.hh             \
# DumpFile.cc                   \
# DumpFile.hh                   \
# Master.cc                     \
# Master.hh                     \
# NanoCubeQueryResult.cc        \
# Query.cc                      \
# QueryParser.cc                \
# geometry.cc                   \
# json.cc                       \
# maps.cc                       \
# mongoose/mongoose.c           \
# vector.cc

# test_words_SOURCES = \
# Heap.cc              \
# PartialSort.cc       \
# test_words.cc 

# nanocube_daemon_SOURCES =    \
# nanocube-daemon.cc           

# ncserve_SOURCES =     \
# ncserve.cc            \
# DumpFile.cc           \
# mongoose/mongoose.c

# nanocube_sliding_SOURCES = \
# nanocube-sliding.cc        \
# DumpFile.cc                \
# NanoCubeSchema.cc          \
# NanoCubeQueryResult.cc     \
# Master.cc                  \
# Query.cc                   \
# QueryParser.cc             \
# geometry.cc                \
# json.cc                    \
# maps.cc                    \
# mongoose/mongoose.c        \
# vector.cc




#
#ncmaster_SOURCES =    \
#DumpFile.cc           \
#Master.cc             \
#NanoCubeQueryResult.cc\
#NanoCubeSchema.cc     \
#Query.cc              \
#QueryParser.cc        \
#geometry.cc           \
#json.cc               \
#maps.cc               \
#mongoose/mongoose.c   \
#ncmaster.cc           \
#vector.cc
#

nc_SOURCES =              \
cache.cc                  \
cache.hh                  \
Common.cc                 \
Common.hh                 \
ContentHolder.hh          \
DumpFile.cc               \
DumpFile.hh               \
FlatTree.hh               \
MemoryUtil.cc             \
MemoryUtil.hh             \
MercatorProjection.cc     \
MercatorProjection.hh     \
NanoCube.hh               \
NanoCubeInsert.hh         \
NanoCubeQuery.hh          \
NanoCubeQueryException.cc \
NanoCubeQueryException.hh \
NanoCubeQueryResult.hh    \
NanoCubeQueryResult.cc    \
NanoCubeSummary.hh        \
NanoCubeSummary.cc        \
NanoCubeReportBuilder.hh  \
NanoCubeSchema.cc         \
NanoCubeSchema.hh         \
NanoCubeTimeQuery.hh      \
QuadTree.cc               \
QuadTree.hh               \
QuadTreeNode.cc           \
QuadTreeNode.hh           \
QuadTreeUtil.hh           \
Query.cc                  \
Query.hh                  \
QueryParser.cc            \
QueryParser.hh            \
QueryResult.cc            \
QueryResult.hh            \
Report.cc                 \
Report.hh                 \
json.cc                   \
json.hh                   \
nanocube_language.cc      \
nanocube_language.hh      \
maps.cc                   \
maps.hh                   \
geometry.cc               \
geometry.hh               \
Server.cc                 \
Server.hh		  \
Stopwatch.cc              \
Stopwatch.hh              \
Stopwatch.hh              \
TaggedPointer.hh          \
TimeBinFunction.cc        \
TimeBinFunction.hh        \
TimeSeries.cc             \
TimeSeries.hh             \
TimeSeriesEntryType.hh    \
Tuple.hh                  \
Util.cc                   \
Util.hh                   \
tile.hh                   \
cache2.hh                 \
mongoose/mongoose.c       \
nc.cc                     \
small_vector.hh           \
qtfilter.hh               \
qtfilter.cc               \
tree_store.hh             \
tree_store.cc             \
tree_store_nanocube.hh    \
tree_store_nanocube.cc    \
tile.cc                   \
tile.hh                   \
address.cc                \
address.hh                \
util/tokenizer.cc         \
util/tokenizer.hh         \
util/timer.cc             \
util/timer.hh             \
util/signal.hh            \
geom2d/base.cc            \
geom2d/base.hh		  \
geom2d/boundingbox.cc	  \
geom2d/boundingbox.hh	  \
geom2d/geom2d.hh	  \
geom2d/make_monotone.cc	  \
geom2d/make_monotone.hh	  \
geom2d/planegraph.cc      \
geom2d/planegraph.hh	  \
geom2d/point.cc		  \
geom2d/point.hh		  \
geom2d/polygon.cc         \
geom2d/polygon.hh	  \
geom2d/tile.cc		  \
geom2d/tile.hh            \
polycover/area.cc              \
polycover/area.hh		  \
polycover/geometry.cc	  \
polycover/geometry.hh	  \
polycover/infix_iterator.hh	  \
polycover/labeled_tree.cc	  \
polycover/labeled_tree.hh	  \
polycover/maps.cc		  \
polycover/maps.hh		  \
polycover/mipmap.cc		  \
polycover/mipmap.hh		  \
polycover/polycover.cc	  \
polycover/polycover.hh	  \
polycover/signal.hh		  \
polycover/tokenizer.cc	  \
polycover/tokenizer.hh


NANOCUBE_VERSION = $(shell cat $(top_srcdir)/VERSION)
AM_CXXFLAGS += -DNANOCUBE_VERSION=\"$(NANOCUBE_VERSION)\"

# NC_LIBS = -ldl -lpthread -lboost_thread -lboost_system
#NC_LIBS = -ldl -lpthread -lboost_thread-mt -lboost_system-mt


nc_q25_u2_u4_LDFLAGS  = $(AM_LDFLAGS)
nc_q25_u2_u4_CXXFLAGS = $(AM_CXXFLAGS) \
    -D_GLIBCXX_USE_NANOSLEEP \
    -D_GLIBCXX_USE_SCHED_YIELD \
    -DLIST_DIMENSION_NAMES=q25 \
    -DLIST_VARIABLE_TYPES=u2,u4 \
    -DVERSION=\"$(VERSION)\"
nc_q25_u2_u4_SOURCES =\
    $(nc_SOURCES)

nc_q25_c1_u2_u4_LDFLAGS  = $(AM_LDFLAGS)
nc_q25_c1_u2_u4_CXXFLAGS = $(AM_CXXFLAGS) \
    -D_GLIBCXX_USE_NANOSLEEP \
    -D_GLIBCXX_USE_SCHED_YIELD \
    -DLIST_DIMENSION_NAMES=q25,c1 \
    -DLIST_VARIABLE_TYPES=u2,u4 \
    -DVERSION=\"$(VERSION)\"
nc_q25_c1_u2_u4_SOURCES =\
    $(nc_SOURCES)

nc_q25_c4_u2_u4_LDFLAGS  = $(AM_LDFLAGS)
nc_q25_c4_u2_u4_CXXFLAGS = $(AM_CXXFLAGS) \
    -D_GLIBCXX_USE_NANOSLEEP \
    -D_GLIBCXX_USE_SCHED_YIELD \
    -DLIST_DIMENSION_NAMES=q25,c4 \
    -DLIST_VARIABLE_TYPES=u2,u4 \
    -DVERSION=\"$(VERSION)\"
nc_q25_c4_u2_u4_SOURCES =\
    $(nc_SOURCES)

nc_q25_c2_u2_u4_LDFLAGS  = $(AM_LDFLAGS)
nc_q25_c2_u2_u4_CXXFLAGS = $(AM_CXXFLAGS) \
    -D_GLIBCXX_USE_NANOSLEEP \
    -D_GLIBCXX_USE_SCHED_YIELD \
    -DLIST_DIMENSION_NAMES=q25,c2 \
    -DLIST_VARIABLE_TYPES=u2,u4 \
    -DVERSION=\"$(VERSION)\"
nc_q25_c2_u2_u4_SOURCES =\
    $(nc_SOURCES)

nc_q25_c1_u2_u8_LDFLAGS  = $(AM_LDFLAGS)
nc_q25_c1_u2_u8_CXXFLAGS = $(AM_CXXFLAGS) \
    -D_GLIBCXX_USE_NANOSLEEP \
    -D_GLIBCXX_USE_SCHED_YIELD \
    -DLIST_DIMENSION_NAMES=q25,c1 \
    -DLIST_VARIABLE_TYPES=u2,u8 \
    -DVERSION=\"$(VERSION)\"
nc_q25_c1_u2_u8_SOURCES =\
    $(nc_SOURCES)

nc_q25_c1_u4_u8_LDFLAGS  = $(AM_LDFLAGS)
nc_q25_c1_u4_u8_CXXFLAGS = $(AM_CXXFLAGS) \
    -D_GLIBCXX_USE_NANOSLEEP \
    -D_GLIBCXX_USE_SCHED_YIELD \
    -DLIST_DIMENSION_NAMES=q25,c1 \
    -DLIST_VARIABLE_TYPES=u4,u8 \
    -DVERSION=\"$(VERSION)\"
nc_q25_c1_u4_u8_SOURCES =\
    $(nc_SOURCES)

nc_q25_c1_c1_u2_u4_LDFLAGS  = $(AM_LDFLAGS)
nc_q25_c1_c1_u2_u4_CXXFLAGS = $(AM_CXXFLAGS) \
    -D_GLIBCXX_USE_NANOSLEEP \
    -D_GLIBCXX_USE_SCHED_YIELD \
    -DLIST_DIMENSION_NAMES=q25,c1,c1 \
    -DLIST_VARIABLE_TYPES=u2,u4 \
    -DVERSION=\"$(VERSION)\"
nc_q25_c1_c1_u2_u4_SOURCES =\
    $(nc_SOURCES)

nc_q25_c1_c1_c1_u2_u4_LDFLAGS  = $(AM_LDFLAGS)
nc_q25_c1_c1_c1_u2_u4_CXXFLAGS = $(AM_CXXFLAGS) \
    -D_GLIBCXX_USE_NANOSLEEP \
    -D_GLIBCXX_USE_SCHED_YIELD \
    -DLIST_DIMENSION_NAMES=q25,c1,c1,c1 \
    -DLIST_VARIABLE_TYPES=u2,u4 \
    -DVERSION=\"$(VERSION)\"
nc_q25_c1_c1_c1_u2_u4_SOURCES =\
    $(nc_SOURCES)

nc_q25_c1_c1_c1_c1_u2_u4_LDFLAGS  = $(AM_LDFLAGS)
nc_q25_c1_c1_c1_c1_u2_u4_CXXFLAGS = $(AM_CXXFLAGS) \
    -D_GLIBCXX_USE_NANOSLEEP \
    -D_GLIBCXX_USE_SCHED_YIELD \
    -DLIST_DIMENSION_NAMES=q25,c1,c1,c1,c1 \
    -DLIST_VARIABLE_TYPES=u2,u4 \
    -DVERSION=\"$(VERSION)\"
nc_q25_c1_c1_c1_c1_u2_u4_SOURCES =\
    $(nc_SOURCES)

nc_q25_c1_c1_c1_c1_c1_u2_u4_LDFLAGS  = $(AM_LDFLAGS)
nc_q25_c1_c1_c1_c1_c1_u2_u4_CXXFLAGS = $(AM_CXXFLAGS) \
    -D_GLIBCXX_USE_NANOSLEEP \
    -D_GLIBCXX_USE_SCHED_YIELD \
    -DLIST_DIMENSION_NAMES=q25,c1,c1,c1,c1,c1 \
    -DLIST_VARIABLE_TYPES=u2,u4 \
    -DVERSION=\"$(VERSION)\"
nc_q25_c1_c1_c1_c1_c1_u2_u4_SOURCES =\
    $(nc_SOURCES)


nc_q20_q20_u4_u4_LDFLAGS  = $(AM_LDFLAGS)
nc_q20_q20_u4_u4_CXXFLAGS = $(AM_CXXFLAGS) \
    -D_GLIBCXX_USE_NANOSLEEP \
    -D_GLIBCXX_USE_SCHED_YIELD \
    -DLIST_DIMENSION_NAMES=q20,q20 \
    -DLIST_VARIABLE_TYPES=u4,u4 \
    -DVERSION=\"$(VERSION)\"
nc_q20_q20_u4_u4_SOURCES =\
    $(nc_SOURCES)
