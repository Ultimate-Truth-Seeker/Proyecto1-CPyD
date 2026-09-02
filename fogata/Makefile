CXX      ?= g++
CXXFLAGS_BASE := -std=c++17 -O3 -ffast-math -march=native -Wall -Wextra -Iinclude
CXXFLAGS := $(CXXFLAGS_BASE)
LDLIBS   := -lm

# OpenMP support (set BUILD_TYPE=parallel to enable)
BUILD_TYPE ?= sequential
ifeq ($(BUILD_TYPE),parallel)
  CXXFLAGS += -fopenmp
  TARGET_SUFFIX := -parallel
else
  TARGET_SUFFIX := -sequential
endif

SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs   sdl2 2>/dev/null)
SDL_LIBS   := $(filter-out -mwindows,$(SDL_LIBS))
ifeq ($(strip $(SDL_LIBS)),)
  ifeq ($(OS),Windows_NT)
    SDL_LIBS := -lmingw32 -lSDL2main -lSDL2
  else
    SDL_LIBS := -lSDL2
  endif
endif

SRC_DIR   := src
BUILD_DIR := build$(TARGET_SUFFIX)
TARGET    := $(BUILD_DIR)/fogata

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all sequential parallel run clean

all: sequential

sequential:
	@echo "Building SEQUENTIAL version (pragmas disabled)..."
	$(MAKE) BUILD_TYPE=sequential _build

parallel:
	@echo "Building PARALLEL version (OpenMP enabled)..."
	$(MAKE) BUILD_TYPE=parallel _build

_build: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(SDL_LIBS) $(LDLIBS)

run: sequential
	./$(BUILD_DIR)/fogata

run-parallel: parallel
	./$(BUILD_DIR)/fogata

clean:
	rm -rf build-sequential build-parallel
