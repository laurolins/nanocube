registry: FORCE
	clang++ -g -mmacosx-version-min=10.11 -std=c++11 -stdlib=libc++ utf8.cc registry.cc -o registry /usr/local/lib/libnanomsg.a

FORCE:

all:
	gcc -O2 -c mongoose.c -I.
	g++ -O2 -c Server.cc -I. -std=c++11
	g++ -O2 -c registry.cc -I. -std=c++11
	g++ -O2 -o registry mongoose.o Server.o registry.o  -ldl -lpthread  -std=c++11


clean:
	rm registry mongoose.o Server.o registry.o
