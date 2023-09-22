# Define any compile-time flags
CXXFLAGS = -O1 -std=c++20 -march=native

# Define the executable file
EXE = main

# Define platform and associated settings
ifeq ($(OS),Windows_NT)
    PLATFORM = WINDOWS
    CXX = clang++
    RM = del /Q
    EXE := $(EXE).exe
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM = LINUX
        CXX = clang++-15
        RM = rm -f
        EXE := $(EXE).out
    endif
endif

# The default rule, to build the executable
all: $(EXE)

$(EXE): main.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

.PHONY: run
run: $(EXE)
	./$(EXE)

.PHONY : clean
clean:
	$(RM) $(EXE)
