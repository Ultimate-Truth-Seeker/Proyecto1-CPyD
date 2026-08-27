# Makefile del screensaver de fogata (version secuencial).
# Uso: make          -> compila build/fogata
#      make run      -> compila y ejecuta con los valores por defecto
#      make clean    -> borra los objetos y el binario

CXX      ?= g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude
LDLIBS   := -lm

# SDL2: preferimos pkg-config; si no esta disponible caemos al enlace manual
# que funciona tanto en MSYS2/MinGW como en Linux.
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs   sdl2 2>/dev/null)
ifeq ($(strip $(SDL_LIBS)),)
  ifeq ($(OS),Windows_NT)
    SDL_LIBS := -lmingw32 -lSDL2main -lSDL2
  else
    SDL_LIBS := -lSDL2
  endif
endif

SRC_DIR   := src
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/fogata

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all run clean

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(SDL_LIBS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
