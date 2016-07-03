all: check_cl use_cl simple
.PHONY: all

use_cl:	src/*
	clang++ -std=c++11 -O2 -o use_cl -Isrc src/main_cl.cpp -lOpenCL -lglfw -ldl

check_cl:	src/check_cl.cpp src/defer.h
		clang++ -std=c++11 -O2 -o check_cl -Isrc src/check_cl.cpp -lOpenCL

simple:	src/main_simple.cpp src/load_shader.cpp src/load_shader.h src/typedefs.h src/defer.h
	clang++ -std=c++11 -O2 -o simple -Isrc src/main_simple.cpp -lglfw -ldl

.PHONY: clean
clean:
	rm check_cl simple use_cl
