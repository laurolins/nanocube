all:
	gcc -O2 -c mongoose.c -I.
	g++ -O2 -c Server.cc -I. -std=c++11
	g++ -O2 -c registry.cc -I. -std=c++11
	g++ -O2 -o registry mongoose.o Server.o registry.o  -ldl -lpthread  -std=c++11


clean:
	rm registry mongoose.o Server.o registry.o
