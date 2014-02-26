LOCAL=$$system(echo $HOME)/local

QMAKE_CFLAGS_X86_64   -= -mmacosx-version-min=10.5
QMAKE_CXXFLAGS_X86_64 -= -mmacosx-version-min=10.5
QMAKE_LFLAGS_X86_64   -= -mmacosx-version-min=10.5
QMAKE_LFLAGS          -= -mmacosx-version-min=10.5

#QMAKE_CXX         = g++-4.8
#QMAKE_CC          = gcc-4.8

# $$system(echo /Users/lauro)

macx {
# mac only
# -stdlib=libc++
# -stdlib=libc++
QMAKE_CXXFLAGS  += -stdlib=libc++
QMAKE_LFLAGS    += -stdlib=libc++
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

LIBS        += -L$${LOCAL}/lib -ldl
INCLUDEPATH += .

HEADERS = \
log.hh \
simple_nanocube.hh \
simple_nanocube_iterator.hh \
report.hh

SOURCES =  \
log.cc \
main.cc \
simple_nanocube.cc \
simple_nanocube_iterator.cc \
report.cc

#// test_simple_nanocube.cc

# main.cc



