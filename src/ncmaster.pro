LOCAL=$$system(echo $HOME)/local

macx {
# mac only
# -stdlib=libc++
# -stdlib=libc++
#QMAKE_CXXFLAGS  += -stdlib=libc++
#QMAKE_LFLAGS    += -stdlib=libc++
QMAKE_CXXFLAGS  += -std=c++11 -mmacosx-version-min=10.7 -Wno-unused-parameter -I$${LOCAL}/include -ftemplate-depth-512
QMAKE_LFLAGS    += -std=c++11 -mmacosx-version-min=10.7 -Wno-unused-parameter
}
unix:!macx{
# linux only
QMAKE_CXXFLAGS  += -std=c++11 -I$${LOCAL}/include -Wunused-parameter
QMAKE_LFLAGS    += -std=c++11
}

LIBS   -= -lQtGui -lQtCore

LIBS        += -L$${LOCAL}/lib -L/usr/local/lib -ldl -lpthread -lz -lboost_thread -lboost_system
INCLUDEPATH += . /usr/local/include


# example 1
#QMAKE_CXXFLAGS  += -DLOCALRUN -DxFLATTREE_VECTOR -DxTIMESERIES_VECTOR -D_GLIBCXX_USE_NANOSLEEP \
#   -DSUPER_FAST_COMPILE -DCOLLECT_MEMUSAGE -DVERSION=2013.09.18T13:43 \
#   $${BUG}

HEADERS =                 \
Master.hh \
    vector.hh \
    DumpFile.hh \
    json.hh \
    NanoCubeQueryResult.hh \
    NanoCubeSchema.hh \
    mongoose.h \
    maps.hh \
    geometry.hh \
    Query.hh \
    QueryParser.hh

SOURCES =                    \
Master.cc                    \
ncmaster.cc \
    vector.cc \
    DumpFile.cc \
    json.cc \
    NanoCubeQueryResult.cc \
    NanoCubeSchema.cc \
    mongoose.c \
    maps.cc \
    geometry.cc \
    Query.cc \
    QueryParser.cc

# stree_serve.cc
# QuadTreeSerialization.cc     \
# test_quadtree_add.cc
# test_quadtree_range_query.cc
# dump_landlines.cc
# dump_landlines.cc
# test_load_and_build_tile.cc
# test_search_stree.cc
# serve.cc
# test_qt_serialization.cc
# test_stree_serialization.cc
# points2qtts3.cc
# points2qt.cc
# test_search_stree.cc
# serve.cc \
# mongoose.c
# STreeSerialization.hh \
# FlatTreeSerialization.hh \
# TimeSeriesSerialization.hh \
# QuadTreeSerialization.hh \

