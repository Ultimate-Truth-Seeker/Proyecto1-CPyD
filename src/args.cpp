#include "args.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

void printUsage(const char* programName) {
    std::printf("Uso: %s -n <cantidad_cuerpos> [opciones]\n\n", programName);
    std::printf("Opciones:\n");
    std::printf("  -n <int>        Cantidad de cuerpos a simular (OBLIGATORIO, > 0)\n");
    std::printf("  -w <int>        Ancho del canvas en pixeles (default 800, minimo 640)\n");
    std::printf("  -ht <int>       Alto del canvas en pixeles (default 600, minimo 480)\n");
    std::printf("  -dt <double>    Paso de tiempo de la simulacion (default 0.01, > 0)\n");
    std::printf("  -seed <uint>    Semilla pseudoaleatoria (default: hora actual)\n");
    std::printf("  -threads <int>  Cantidad de hilos OpenMP (default 0 = automatico)\n");
    std::printf("  -h, --help      Muestra esta ayuda\n\n");
    std::printf("Ejemplo: %s -n 500 -w 1024 -ht 768 -threads 4\n", programName);
}

// Intenta parsear el string s como entero positivo. Retorna true si es valido.
static bool parsePositiveInt(const char* s, int& out) {
    if (s == nullptr || s[0] == '\0') return false;
    char* endPtr = nullptr;
    long val = std::strtol(s, &endPtr, 10);
    if (*endPtr != '\0') return false;  // el string tenia caracteres no numericos
    if (val <= 0) return false;
    out = static_cast<int>(val);
    return true;
}

// Intenta parsear el string s como double positivo. Retorna true si es valido.
static bool parsePositiveDouble(const char* s, double& out) {
    if (s == nullptr || s[0] == '\0') return false;
    char* endPtr = nullptr;
    double val = std::strtod(s, &endPtr);
    if (*endPtr != '\0') return false;
    if (val <= 0.0) return false;
    out = val;
    return true;
}

bool parseArgs(int argc, char** argv, SimConfig& config, std::string& errorMsg) {
    bool nProvided = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        // Cada flag consume el siguiente argv como su valor; validamos que
        // exista un "siguiente" argumento antes de leerlo (programacion
        // defensiva contra "-n" sin valor al final de la linea de comando).
        auto hasNext = [&](void) { return i + 1 < argc; };

        if (arg == "-h" || arg == "--help") {
            errorMsg = "help";
            return false;
        } else if (arg == "-n") {
            if (!hasNext() || !parsePositiveInt(argv[++i], config.n)) {
                errorMsg = "-n requiere un entero positivo (cantidad de cuerpos)";
                return false;
            }
            nProvided = true;
        } else if (arg == "-w") {
            if (!hasNext() || !parsePositiveInt(argv[++i], config.width)) {
                errorMsg = "-w requiere un entero positivo (ancho del canvas)";
                return false;
            }
        } else if (arg == "-ht") {
            if (!hasNext() || !parsePositiveInt(argv[++i], config.height)) {
                errorMsg = "-ht requiere un entero positivo (alto del canvas)";
                return false;
            }
        } else if (arg == "-dt") {
            if (!hasNext() || !parsePositiveDouble(argv[++i], config.dt)) {
                errorMsg = "-dt requiere un numero decimal positivo (paso de tiempo)";
                return false;
            }
        } else if (arg == "-seed") {
            int seedVal;
            if (!hasNext() || !parsePositiveInt(argv[++i], seedVal)) {
                errorMsg = "-seed requiere un entero positivo";
                return false;
            }
            config.seed = static_cast<unsigned int>(seedVal);
        } else if (arg == "-threads") {
            if (!hasNext() || !parsePositiveInt(argv[++i], config.threads)) {
                errorMsg = "-threads requiere un entero positivo";
                return false;
            }
        } else {
            errorMsg = "Argumento desconocido: " + arg;
            return false;
        }
    }

    // -n es obligatorio segun el enunciado del proyecto (al menos un parametro N)
    if (!nProvided) {
        errorMsg = "Falta el argumento obligatorio -n <cantidad_cuerpos>";
        return false;
    }

    // Requisitos de la guia: canvas minimo 640x480
    if (config.width < 640) {
        errorMsg = "El ancho minimo del canvas es 640 (recibido: " +
                   std::to_string(config.width) + ")";
        return false;
    }
    if (config.height < 480) {
        errorMsg = "El alto minimo del canvas es 480 (recibido: " +
                   std::to_string(config.height) + ")";
        return false;
    }

    // Si no se dio semilla explicita, usar la hora actual para que cada
    // corrida sin -seed genere una galaxia distinta.
    if (config.seed == 0) {
        config.seed = static_cast<unsigned int>(std::time(nullptr));
    }

    return true;
}
