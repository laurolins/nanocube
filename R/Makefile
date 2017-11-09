# SRC=\
# nanocube_platform.c \
# nanocube_alloc.c \
# nanocube_time.c \
# nanocube_btree.c \
# nanocube_vector_payload.h \
# nanocube_index.c \
# nanocube_tokenizer.c \
# nanocube_time_parser.c \
# nanocube_parser.c \
# nanocube_measure.c \
# nanocube_vector.c \
# win32_nanocube_platform.c \
# osx_nanocube_platform.c

all: $(SRC)
	@echo "Copied source files to base"

clean:
	rm -f nanocube/src/base/*.h
	rm -f nanocube/src/base/*.c

%.c:
	cp ../src/$@ nanocube/src/base/.
%.h:
	cp ../src/$@ nanocube/src/base/.

pkg: clean all
	R CMD build nanocube
	R --arch=x86_64 CMD check --no-multiarch nanocube_0.0.1.tar.gz

install: clean all
	rsync -av ../src nanocube/src/.
	R CMD build nanocube
	R --arch=x86_64 CMD INSTALL --no-multiarch nanocube_0.0.1.tar.gz

osx: clean all
	rsync -av ../src nanocube/src/.
	R CMD build nanocube
	R CMD INSTALL --no-multiarch nanocube_0.0.1.tar.gz

linux: clean all
	rsync -av ../src nanocube/src/.
	R CMD build nanocube
	R CMD INSTALL --no-multiarch nanocube_0.0.1.tar.gz

# BASE=$(shell pwd)
# VERSION=$(shell cat nanocube/DESCRIPTION | grep Version | cut -d" " -f 2)
# all:
# 	rm -rf /tmp/nanocube-${VERSION}.tar.gz; \
# 	tar cvfhz /tmp/nanocube-${VERSION}.tar.gz nanocube ; \
# 	cd /tmp ; \
# 	tar xvfz nanocube-${VERSION}.tar.gz; \
# 	find /tmp/nanocube | egrep "([.]o|[.]so|CMakeLists.txt)$$" | xargs rm ; \
# 	tar cvfz nanocube-${VERSION}.tar.gz nanocube; \
# 	cd ${BASE} ; \
# 	mv /tmp/nanocube-${VERSION}.tar.gz .
