

simple:	src/*
	clang++ -std=c++11 -O2 -o simple -Isrc src/main_simple.cpp -lglfw -ldl
