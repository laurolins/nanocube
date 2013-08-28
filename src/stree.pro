LOCAL=$$system(echo $HOME)/local

# $$system(echo /Users/lauro)

macx {
# mac only
# -stdlib=libc++
# -stdlib=libc++
QMAKE_CXXFLAGS  += -std=c++11 -mmacosx-version-min=10.7 -Wno-unused-parameter -I$${LOCAL}/include
QMAKE_LFLAGS    += -std=c++11 -mmacosx-version-min=10.7 -Wno-unused-parameter
}
unix:!macx{
# linux only
QMAKE_CXXFLAGS  += -std=c++11 -I$${LOCAL}/include -Wunused-parameter
QMAKE_LFLAGS    += -std=c++11
}

QT     -=  gui core
LIBS   -= -lQtGui -lQtCore

LIBS        += -L$${LOCAL}/lib -lserialization -ldl -lpthread
INCLUDEPATH += .


QMAKE_CXXFLAGS  += -DLOCALRUN -DxFLATTREE_VECTOR -DxTIMESERIES_VECTOR -D_GLIBCXX_USE_NANOSLEEP -DFAST_COMPILE -DCOLLECT_MEMUSAGE -DVERSION=\\\"$$system(cat ../VERSION)\\\"
# -O3

HEADERS =                 \
MercatorProjection.hh     \
QuadTree.hh               \
QuadTreeNode.hh \
QuadTreeUtil.hh \
Stopwatch.hh \
TimeSeries.hh \
Common.hh \
STree.hh \
FlatTree.hh \
ContentHolder.hh \
small_vector.hh \
TaggedPointer.hh \
MemoryUtil.hh \
Stopwatch.hh  \
TimeBinFunction.hh \
Server.hh \
Util.hh \
Tuple.hh

#STreeSerialization.hh \
#FlatTreeSerialization.hh \
#TimeSeriesSerialization.hh \
#QuadTreeSerialization.hh \


SOURCES =                    \
Stopwatch.cc                 \
MemoryUtil.cc                \
QuadTreeNode.cc              \
TimeSeries.cc                \
Common.cc \
QuadTree.cc                  \
MercatorProjection.cc        \
STree.cc                     \
Util.cc                      \
TimeBinFunction.cc           \
Server.cc                    \
mongoose.c                   \
stree_serve.cc

#QuadTreeSerialization.cc     \


#test_quadtree_add.cc

# --port=29700 --tbin=2010_1h --levels=20 --max=1 < nytaxi_K.dmp

# dump_taxi.cc



# test_quadtree_range_query.cc



# dump_landlines.cc


# dump_landlines.cc




#


# dump_mts.cc

# test_load_and_build_tile.cc

#


#test_search_stree.cc


#

# serve.cc
# test_qt_serialization.cc
# test_stree_serialization.cc

# points2qtts3.cc


#points2qt.cc

#test_search_stree.cc

#
#serve.cc \
#mongoose.c
