# Path to the matrix library
RGB_LIB_DISTRIBUTION=rpi-rgb-led-matrix
INCDIR=$(RGB_LIB_DISTRIBUTION)/include
LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix

# Compiler Settings
CXX=g++
CXXFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter
CXXFLAGS+=-I$(INCDIR)

# Linker Settings
LDFLAGS+=-L$(LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread -lstdc++

# Build Rules
all: wordle_game

# Single-step compilation (Direct to executable, no .o file)
wordle_game: wordle_game.cpp
	$(CXX) $(CXXFLAGS) wordle_game.cpp -o wordle_game $(LDFLAGS)

clean:
	rm -f wordle_game