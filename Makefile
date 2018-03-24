
all : horn

run : horn
	./horn

horn : main.cpp generate.cpp utils.cpp horn.hpp
	g++ -g -std=c++11 -Wall main.cpp -o horn -lpthread

release: main.cpp generate.cpp utils.cpp horn.hpp
	g++ -std=c++11 -Wall -O3 main.cpp -o horn -lpthread