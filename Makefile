CXX = g++-14
STDFLAGS = -std=gnu++23


bf: src/main.cc src/typedefs.h
	$(CXX) src/main.cc $(STDFLAGS) -o bf


opt: src/main.cc src/typedefs.h
	$(CXX) src/main.cc $(STDFLAGS) -O2 -o bf



