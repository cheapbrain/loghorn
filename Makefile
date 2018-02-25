
all : horn

horn : main.cpp
	g++ -g -std=c++11 -Wall main.cpp -o horn
