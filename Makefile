release:	src/*
		clang++ -std=c++11 -O2 -o release -Isrc src/main.cpp -lglfw -ldl

debug:	src/*
	clang++ -std=c++11 -O0 -g -fstandalone-debug -o debug -I src src/main.cpp -lglfw -ldl
