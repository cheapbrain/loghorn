
all : horn

horn : main.cpp generate.cpp utils.cpp horn.hpp
	g++ -g -std=c++11 -Wall main.cpp -o horn -lpthread
