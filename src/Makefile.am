#EXTRA_DATA = extra
#extradir = $(prefix)/bin
#extra_DATA = ncbuild

install-exec-hook:
	cp $(top_srcdir)/src/ncbuild $(prefix)/bin
	chmod +x $(prefix)/bin/ncbuild


bin_PROGRAMS =        \
ncdmp                 \
ncserve               \
nc_q20_q20_u2_u4      \
nc_q20_q20_u3_u4      \
nc_q20_u2_u4

# AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -D_GLIBCXX_USE_NANOSLEEP

ncdmp_SOURCES =       \
ncdmp.cc              \
ncdmp_base.cc         \
ncdmp_base.hh         \
TimeBinFunction.cc    \
TimeBinFunction.hh    \
DumpFile.cc           \
DumpFile.hh           \
MercatorProjection.cc \
MercatorProjection.hh

ncserve_SOURCES =     \
ncserve.cc            \
DumpFile.cc           \
DumpFile.hh

nc_SOURCES = \
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
vector.cc                 \
vector.hh                 \
mongoose.c                \
nc.cc                     \
small_vector.hh


nc_q20_q20_u2_u4_LDFLAGS  = -ldl -lpthread
nc_q20_q20_u2_u4_CXXFLAGS = \
   -D_GLIBCXX_USE_NANOSLEEP \
    -DLIST_DIMENSION_NAMES=q20,q20 \
    -DLIST_VARIABLE_TYPES=u2,u4 
nc_q20_q20_u2_u4_SOURCES =\
    $(nc_SOURCES)

nc_q20_q20_u3_u4_LDFLAGS  = -ldl -lpthread
nc_q20_q20_u3_u4_CXXFLAGS = \
   -D_GLIBCXX_USE_NANOSLEEP \
    -DLIST_DIMENSION_NAMES=q20,q20 \
    -DLIST_VARIABLE_TYPES=u3,u4 
nc_q20_q20_u3_u4_SOURCES =\
    $(nc_SOURCES)

nc_q20_u2_u4_LDFLAGS  = -ldl -lpthread
nc_q20_u2_u4_CXXFLAGS = \
   -D_GLIBCXX_USE_NANOSLEEP \
    -DLIST_DIMENSION_NAMES=q20 \
    -DLIST_VARIABLE_TYPES=u2,u4 
nc_q20_u2_u4_SOURCES =\
    $(nc_SOURCES)
