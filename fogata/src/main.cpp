// main.cpp — Punto de entrada del screensaver de fogata (version secuencial).
//
// Flujo del programa:
//   1. Captura y validacion de argumentos (programacion defensiva).
//   2. Inicializacion de SDL, ventana y sistema de particulas.
//   3. Bucle principal: eventos -> fisica -> render -> medicion de FPS.
//   4. Liberacion ordenada de todos los recursos.
#include <SDL2/SDL.h>

#include <cstdio>
#include <exception>
#include <new>
#include <string>

#include "config.h"
#include "particle.h"
#include "renderer.h"

namespace {

// Cotas del paso de tiempo. Si el sistema operativo nos deja congelados un
// instante, un dt gigante volaria las particulas fuera de pantalla; y un dt
// de cero haria que la escena se quedara estatica.
constexpr float kMinDeltaTime = 1.0f / 1000.0f;
constexpr float kMaxDeltaTime = 1.0f / 15.0f;

// Cada cuanto se refresca el contador de FPS mostrado, en segundos.
constexpr double kFpsWindow = 0.35;

// Procesa la cola de eventos de SDL.
// Retorna false cuando el usuario pide cerrar el screensaver.
bool handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) return false;
        if (event.type == SDL_KEYDOWN) {
            const SDL_Keycode key = event.key.keysym.sym;
            if (key == SDLK_ESCAPE || key == SDLK_q) return false;
        }
    }
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    // --- 1. Captura de argumentos ---
    Config      config;
    std::string error;
    const char* programName = (argc > 0 && argv[0] != nullptr) ? argv[0] : "fogata";

    if (!configParse(argc, argv, config, error)) {
        std::fprintf(stderr, "Error: %s\n\n", error.c_str());
        configPrintUsage(programName);
        return 1;
    }
    if (config.showHelp) {
        configPrintUsage(programName);
        return 0;
    }

    std::printf("Fogata secuencial | N=%d  %dx%d  intensidad=%.2f  viento=%.2f  semilla=%u  vsync=%s\n",
                config.nParticles, config.width, config.height,
                static_cast<double>(config.intensity), static_cast<double>(config.wind),
                config.seed, config.vsync ? "si" : "no");

    // --- 2. Inicializacion ---
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "Error: no se pudo inicializar SDL: %s\n", SDL_GetError());
        return 1;
    }

    int exitCode = 0;
    {
        // Ambito propio: los destructores de Renderer y FireSystem corren
        // antes de SDL_Quit(), que es el orden correcto de liberacion.
        Renderer renderer;
        if (!renderer.init(config, error)) {
            std::fprintf(stderr, "Error: %s\n", error.c_str());
            SDL_Quit();
            return 1;
        }

        try {
            FireSystem fire(config);

            // --- 3. Bucle principal ---
            Uint64 previousTicks = SDL_GetPerformanceCounter();
            const double tickFrequency = static_cast<double>(SDL_GetPerformanceFrequency());

            int    framesInWindow = 0;      // frames acumulados en la ventana de medicion
            double secondsInWindow = 0.0;   // tiempo acumulado en esa ventana
            float  displayedFps = 0.0f;     // ultimo promedio calculado
            double secondsSinceLog = 0.0;   // para el reporte por consola

            bool running = true;
            while (running) {
                running = handleEvents();

                // Paso de tiempo real entre frames, acotado a un rango seguro.
                const Uint64 currentTicks = SDL_GetPerformanceCounter();
                double elapsed = static_cast<double>(currentTicks - previousTicks) / tickFrequency;
                previousTicks = currentTicks;
                float deltaTime = static_cast<float>(elapsed);
                if (deltaTime < kMinDeltaTime) deltaTime = kMinDeltaTime;
                if (deltaTime > kMaxDeltaTime) deltaTime = kMaxDeltaTime;

                fire.update(deltaTime);
                renderer.drawFrame(fire, displayedFps);

                // --- Medicion de FPS ---
                // Promedio sobre una ventana corta: mas estable que 1/dt y
                // suficientemente reactivo para notar caidas de rendimiento.
                ++framesInWindow;
                secondsInWindow += elapsed;
                secondsSinceLog += elapsed;
                if (secondsInWindow >= kFpsWindow) {
                    displayedFps = static_cast<float>(framesInWindow / secondsInWindow);
                    framesInWindow = 0;
                    secondsInWindow = 0.0;
                }
                if (secondsSinceLog >= 1.0) {
                    std::printf("FPS: %.1f\n", static_cast<double>(displayedFps));
                    std::fflush(stdout);
                    secondsSinceLog = 0.0;
                }
            }
        } catch (const std::bad_alloc&) {
            std::fprintf(stderr,
                         "Error: no hay memoria suficiente para %d particulas. "
                         "Pruebe con un valor de -n menor.\n", config.nParticles);
            exitCode = 1;
        } catch (const std::exception& ex) {
            std::fprintf(stderr, "Error inesperado: %s\n", ex.what());
            exitCode = 1;
        }
    }

    // --- 4. Liberacion ---
    SDL_Quit();
    return exitCode;
}
