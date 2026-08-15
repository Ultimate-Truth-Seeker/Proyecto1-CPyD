CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude -fopenmp
SDL_FLAGS = $(shell pkg-config --cflags --libs sdl2)

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

# Objetos compartidos por todo el proyecto
COMMON_OBJS = $(BUILD_DIR)/body.o $(BUILD_DIR)/physics_seq.o \
              $(BUILD_DIR)/integrator.o $(BUILD_DIR)/init.o \
              $(BUILD_DIR)/args.o

.PHONY: all clean test_physics test_renderer test_init

all: test_physics test_renderer test_init

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/renderer.o: $(SRC_DIR)/renderer.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SDL_FLAGS) -c $< -o $@

# --- Entregables de prueba, Semana 1 ---

# Persona A: fisica secuencial + integrador, solo consola
test_physics: $(BUILD_DIR)/body.o $(BUILD_DIR)/physics_seq.o $(BUILD_DIR)/integrator.o
	$(CXX) $(CXXFLAGS) $(TEST_DIR)/test_physics_console.cpp $^ -o $(BUILD_DIR)/test_physics

# Persona B: ventana SDL + parseo de argumentos
test_renderer: $(BUILD_DIR)/args.o $(BUILD_DIR)/renderer.o
	$(CXX) $(CXXFLAGS) $(SDL_FLAGS) $(TEST_DIR)/test_renderer_window.cpp $^ -o $(BUILD_DIR)/test_renderer $(SDL_FLAGS)

# Persona C: generacion de galaxia inicial, solo consola
test_init: $(BUILD_DIR)/body.o $(BUILD_DIR)/init.o
	$(CXX) $(CXXFLAGS) $(TEST_DIR)/test_init_console.cpp $^ -o $(BUILD_DIR)/test_init

clean:
	rm -rf $(BUILD_DIR)
