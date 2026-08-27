// config.cpp — Parseo defensivo de la linea de comandos.
#include "config.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

// Convierte texto a entero rechazando basura ("12abc", "", " ", overflow).
bool parseInt(const char* text, long& out) {
    if (text == nullptr || *text == '\0') return false;
    errno = 0;
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0') return false;
    out = value;
    return true;
}

// Convierte texto a flotante rechazando basura y valores no finitos.
bool parseFloat(const char* text, float& out) {
    if (text == nullptr || *text == '\0') return false;
    errno = 0;
    char* end = nullptr;
    const double value = std::strtod(text, &end);
    if (errno == ERANGE || end == text || *end != '\0') return false;
    out = static_cast<float>(value);
    return true;
}

// Arma el mensaje "la opcion X necesita un valor" de forma uniforme.
std::string missingValue(const char* flag) {
    return std::string("la opcion '") + flag + "' necesita un valor.";
}

}  // namespace

void configPrintUsage(const char* programName) {
    std::fprintf(stderr,
        "Screensaver de fogata (version secuencial)\n"
        "\n"
        "Uso: %s [opciones]\n"
        "\n"
        "  -n  <entero>   Cantidad de particulas de fuego  (%d..%d, por defecto 3000)\n"
        "  -w  <entero>   Ancho de la ventana en pixeles   (min %d, por defecto 1280)\n"
        "  -h  <entero>   Alto de la ventana en pixeles    (min %d, por defecto 720)\n"
        "  -i  <decimal>  Intensidad del brillo            (%.2f..%.2f, por defecto 1.00)\n"
        "  -v  <decimal>  Viento lateral                   (-%.2f..%.2f, por defecto 0.35)\n"
        "  -s  <entero>   Semilla pseudoaleatoria          (0 = usar el reloj)\n"
        "  --no-vsync     Desactiva la sincronia vertical (util para medir FPS reales)\n"
        "  --help         Muestra esta ayuda\n"
        "\n"
        "Durante la ejecucion: ESC o Q para salir.\n",
        programName,
        limits::kMinParticles, limits::kMaxParticles,
        limits::kMinWidth, limits::kMinHeight,
        limits::kMinIntensity, limits::kMaxIntensity,
        limits::kMaxWind, limits::kMaxWind);
}

bool configParse(int argc, char** argv, Config& out, std::string& error) {
    Config cfg;  // arranca con los valores por defecto

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        // Devuelve el siguiente argumento como valor, o nullptr si no existe.
        auto nextValue = [&]() -> const char* {
            return (i + 1 < argc) ? argv[++i] : nullptr;
        };

        if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-?") == 0) {
            cfg.showHelp = true;
            out = cfg;
            return true;
        }
        if (std::strcmp(arg, "--no-vsync") == 0) {
            cfg.vsync = false;
            continue;
        }

        long   asInt   = 0;
        float  asFloat = 0.0f;

        if (std::strcmp(arg, "-n") == 0) {
            const char* value = nextValue();
            if (!value) { error = missingValue("-n"); return false; }
            if (!parseInt(value, asInt)) {
                error = std::string("'-n ") + value + "' no es un entero valido.";
                return false;
            }
            if (asInt < limits::kMinParticles || asInt > limits::kMaxParticles) {
                error = "N fuera de rango: debe estar entre " +
                        std::to_string(limits::kMinParticles) + " y " +
                        std::to_string(limits::kMaxParticles) + ".";
                return false;
            }
            cfg.nParticles = static_cast<int>(asInt);
        } else if (std::strcmp(arg, "-w") == 0) {
            const char* value = nextValue();
            if (!value) { error = missingValue("-w"); return false; }
            if (!parseInt(value, asInt)) {
                error = std::string("'-w ") + value + "' no es un entero valido.";
                return false;
            }
            if (asInt < limits::kMinWidth || asInt > limits::kMaxWidth) {
                error = "ancho fuera de rango: minimo " +
                        std::to_string(limits::kMinWidth) + ", maximo " +
                        std::to_string(limits::kMaxWidth) + ".";
                return false;
            }
            cfg.width = static_cast<int>(asInt);
        } else if (std::strcmp(arg, "-h") == 0) {
            const char* value = nextValue();
            if (!value) { error = missingValue("-h"); return false; }
            if (!parseInt(value, asInt)) {
                error = std::string("'-h ") + value + "' no es un entero valido.";
                return false;
            }
            if (asInt < limits::kMinHeight || asInt > limits::kMaxHeight) {
                error = "alto fuera de rango: minimo " +
                        std::to_string(limits::kMinHeight) + ", maximo " +
                        std::to_string(limits::kMaxHeight) + ".";
                return false;
            }
            cfg.height = static_cast<int>(asInt);
        } else if (std::strcmp(arg, "-i") == 0) {
            const char* value = nextValue();
            if (!value) { error = missingValue("-i"); return false; }
            if (!parseFloat(value, asFloat)) {
                error = std::string("'-i ") + value + "' no es un decimal valido.";
                return false;
            }
            if (asFloat < limits::kMinIntensity || asFloat > limits::kMaxIntensity) {
                error = "intensidad fuera de rango: debe estar entre 0.10 y 5.00.";
                return false;
            }
            cfg.intensity = asFloat;
        } else if (std::strcmp(arg, "-v") == 0) {
            const char* value = nextValue();
            if (!value) { error = missingValue("-v"); return false; }
            if (!parseFloat(value, asFloat)) {
                error = std::string("'-v ") + value + "' no es un decimal valido.";
                return false;
            }
            if (asFloat < -limits::kMaxWind || asFloat > limits::kMaxWind) {
                error = "viento fuera de rango: debe estar entre -3.00 y 3.00.";
                return false;
            }
            cfg.wind = asFloat;
        } else if (std::strcmp(arg, "-s") == 0) {
            const char* value = nextValue();
            if (!value) { error = missingValue("-s"); return false; }
            if (!parseInt(value, asInt) || asInt < 0) {
                error = std::string("'-s ") + value + "' no es una semilla valida (entero >= 0).";
                return false;
            }
            cfg.seed = static_cast<uint32_t>(asInt);
        } else {
            error = std::string("opcion desconocida: '") + arg + "'.";
            return false;
        }
    }

    // Semilla 0 significa "no determinista": la derivamos del reloj de alta
    // resolucion para que cada corrida tenga colores y chispas distintos.
    if (cfg.seed == 0) {
        const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        cfg.seed = static_cast<uint32_t>(now.count()) | 1u;  // xorshift no admite 0
    }

    out = cfg;
    return true;
}
