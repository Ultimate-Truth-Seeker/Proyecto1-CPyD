// config.h — Configuracion del screensaver y parseo defensivo de argumentos.
#ifndef FOGATA_CONFIG_H
#define FOGATA_CONFIG_H

#include <cstdint>
#include <string>

// Limites duros: cualquier valor fuera de rango se rechaza con mensaje de error.
namespace limits {
constexpr int      kMinWidth      = 640;      // requisito del enunciado
constexpr int      kMinHeight     = 480;      // requisito del enunciado
constexpr int      kMaxWidth      = 7680;
constexpr int      kMaxHeight     = 4320;
constexpr int      kMinParticles  = 1;
constexpr int      kMaxParticles  = 2000000;  // techo para no agotar memoria
constexpr float    kMinIntensity  = 0.10f;
constexpr float    kMaxIntensity  = 5.00f;
constexpr float    kMaxWind       = 3.00f;
}  // namespace limits

// Parametros de ejecucion. Todo lo que el usuario puede ajustar vive aqui:
// no hay constantes de ventana ni de conteo regadas por el resto del codigo.
struct Config {
    int      width       = 1280;   // ancho del canvas en pixeles
    int      height      = 720;    // alto del canvas en pixeles
    int      nParticles  = 3000;   // N: cantidad de particulas de fuego
    float    intensity   = 1.0f;   // multiplicador de brillo de la llama
    float    wind        = 0.35f;  // fuerza del viento lateral (rafagas)
    uint32_t seed        = 0;      // semilla PRNG; 0 = derivada del reloj
    bool     vsync       = true;   // sincronizar con el refresco del monitor
    bool     showHelp    = false;  // el usuario pidio --help
};

// Escribe en stderr el modo de uso completo del programa.
void configPrintUsage(const char* programName);

// Parsea argv y valida cada campo.
// Entradas : argc/argv tal cual los recibe main().
// Salidas  : 'out' con la configuracion final, 'error' con el motivo del fallo.
// Retorna  : true si la configuracion es valida y el programa puede continuar.
bool configParse(int argc, char** argv, Config& out, std::string& error);

#endif  // FOGATA_CONFIG_H
