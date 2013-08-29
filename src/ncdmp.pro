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

LIBS        += -L$${LOCAL}/lib -ldl -lpthread -lz
INCLUDEPATH += .

QMAKE_CXXFLAGS  += \
   -D_GLIBCXX_USE_NANOSLEEP

# -O3

HEADERS =             \
DumpFile.hh           \
TimeBinFunction.hh

SOURCES =             \
ncdmp.cc              \
ncdmp_base.cc         \
ncdmp_base.hh         \
TimeBinFunction.cc    \
DumpFile.cc           \
MercatorProjection.cc \
