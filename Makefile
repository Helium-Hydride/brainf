CXX = g++-14
CFLAGS = -std=gnu++23


bf: src/main.cc src/typedefs.h
	$(CXX) src/main.cc $(CFLAGS) -o bf


opt: src/main.cc src/typedefs.h
	$(CXX) src/main.cc $(CFLAGS) -O2 -o bf



