# Kompilátor
CXX = acpp

# Voliteľné: vyber backend (komentuj podľa potreby)
# CXXFLAGS += -fsycl-targets=nvptx64-nvidia-cuda
# CXXFLAGS += -fsycl-targets=omp

CXXFLAGS = -std=c++20 -O2 -Wall -Wextra

INCLUDES = -I./raylib/src -I./raygui/src
LDFLAGS = -lraylib -lm -ldl -lpthread

# Vyber všetky .cpp súbory okrem build/ a raylib/build/ (ignorujeme CMake veci)
SRCS := $(shell find . -type f -name "*.cpp" ! -path "./raylib/build/*")
OBJS := $(SRCS:.cpp=.o)

BIN = build/diplomovka

all: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf build
	find . -type f -name "*.o" -delete

