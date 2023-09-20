CXX = clang++
CXXFLAGS = -Ofast -std=c++20

main.exe: main.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

.PHONY: run
run: main.exe
	./main.exe

.PHONY : clean
clean:
	rm -f *.exe *.o