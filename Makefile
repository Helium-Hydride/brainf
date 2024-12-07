CXX = g++-14
CFLAGS = -std=gnu++23

CLANG = clang++
CLANGFLAGS = -std=c++23 -stdlib=libc++


bf: src/main.cc src/typedefs.h
	$(CXX) src/main.cc $(CFLAGS) -O2 -fwhole-program -march=native -o bf


debug: src/main.cc src/typedefs.h
	$(CXX) src/main.cc $(CFLAGS) -Og -g -o bf


clang: src/main.cc src/typedefs.h
	$(CLANG) src/main.cc $(CLANGFLAGS) -O2 -o bf
