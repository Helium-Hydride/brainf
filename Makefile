CXX = clang++
STDFLAGS = -std=c++23 -stdlib=libc++


bf: src/main.cc src/typedefs.h
	$(CXX) src/main.cc $(STDFLAGS) -o bf


opt: src/main.cc src/typedefs.h
	$(CXX) src/main.cc $(STDFLAGS) -O2 -o bf



