#include <SDL2/SDL.h>

#include <chrono>
#include <cstdio>
#include <exception>
#include <new>
#include <string>

#include "config.h"
#include "particle.h"
#include "renderer.h"

namespace {

constexpr float  kMinDeltaTime = 1.0f / 1000.0f;
constexpr float  kMaxDeltaTime = 1.0f / 15.0f;
constexpr double kFpsWindow    = 0.35;

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

void runScreensaver(Renderer& renderer, FireSystem& fire, Config& config) {
    Uint64 previousTicks = SDL_GetPerformanceCounter();
    const double tickFrequency = static_cast<double>(SDL_GetPerformanceFrequency());

    int    framesInWindow  = 0;
    double secondsInWindow = 0.0;
    double secondsSinceLog = 0.0;
    float  displayedFps    = 0.0f;

    // Acumuladores para instrumentar findCriticalSpark() por separado del
    // FPS general (ver Anexo 3): el costo de esta busqueda es una fraccion
    // pequena del frame, asi que mezclarla con el tiempo de frame completo
    // esconderia el efecto que se quiere medir. Se reporta en la misma
    // ventana de 1 segundo que el log de FPS, pero en su propia linea.
    long long sparkSamples          = 0;
    double    sparkMicrosAccum      = 0.0;
    long long sparkIterationsAccum  = 0;

    while (handleEvents()) {
        const Uint64 currentTicks = SDL_GetPerformanceCounter();
        const double elapsed =
            static_cast<double>(currentTicks - previousTicks) / tickFrequency;
        previousTicks = currentTicks;

        float deltaTime = static_cast<float>(elapsed);
        if (deltaTime < kMinDeltaTime) deltaTime = kMinDeltaTime;
        if (deltaTime > kMaxDeltaTime) deltaTime = kMaxDeltaTime;

        fire.update(deltaTime);

        // Busqueda de la chispa critica, cronometrada de forma aislada.
        // std::chrono::steady_clock no se ve afectado por ajustes del reloj
        // del sistema, a diferencia de system_clock, lo que importa para
        // mediciones de duracion cortas y repetidas como esta.
        const auto sparkStart = std::chrono::steady_clock::now();
        const long long sparkIterations =
            fire.findCriticalSparkWorkload(config.searchWorkload);
        const auto sparkEnd = std::chrono::steady_clock::now();

        sparkMicrosAccum += std::chrono::duration<double, std::micro>(sparkEnd - sparkStart).count();
        sparkIterationsAccum += sparkIterations;
        ++sparkSamples;

        int selectedSearchIterations = 0;
        const int selectedParticle = fire.findCriticalSpark(selectedSearchIterations);
        renderer.drawFrame(fire, displayedFps, selectedParticle);

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
            if (sparkSamples > 0) {
                std::printf("  findCriticalSpark: %.2f us/frame promedio, %.0f iteraciones/frame promedio\n",
                            sparkMicrosAccum / static_cast<double>(sparkSamples),
                            static_cast<double>(sparkIterationsAccum) / static_cast<double>(sparkSamples));
            }
            std::fflush(stdout);
            secondsSinceLog = 0.0;
            sparkSamples = 0;
            sparkMicrosAccum = 0.0;
            sparkIterationsAccum = 0;
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
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

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "Error: no se pudo inicializar SDL: %s\n", SDL_GetError());
        return 1;
    }

    int exitCode = 0;
    {
        Renderer renderer;
        try {
            FireSystem fire(config);
            if (renderer.init(config, fire, error)) {
                runScreensaver(renderer, fire, config);
            } else {
                std::fprintf(stderr, "Error: %s\n", error.c_str());
                exitCode = 1;
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

    SDL_Quit();
    return exitCode;
}
