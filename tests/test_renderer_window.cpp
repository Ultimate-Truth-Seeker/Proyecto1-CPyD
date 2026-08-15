// Entregable Semana 1 - Persona B
//
// Prueba de que SDL abre una ventana correctamente y que el loop de render
// corre sin errores (clear -> present -> handleEvents), usando la
// configuracion leida por el parser de argumentos. Todavia no dibuja
// cuerpos porque el modulo de fisica/inicializacion de galaxia se conecta
// en la semana 2 (integracion). Cierra con ESC o con la X de la ventana.
//
// Compilar y correr: ver README.md / Makefile (target test_renderer)

#include <cstdio>
#include <string>
#include "args.h"
#include "renderer.h"

int main(int argc, char** argv) {
    SimConfig config;
    std::string errorMsg;

    if (!parseArgs(argc, argv, config, errorMsg)) {
        if (errorMsg != "help") {
            std::fprintf(stderr, "Error: %s\n\n", errorMsg.c_str());
        }
        printUsage(argv[0]);
        return errorMsg == "help" ? 0 : 1;
    }

    std::printf("Configuracion cargada: n=%d, canvas=%dx%d, dt=%.4f, seed=%u, threads=%d\n",
                config.n, config.width, config.height, config.dt, config.seed, config.threads);

    Renderer renderer;
    if (!renderer.init(config.width, config.height, "Galaxy Sim - Prueba de ventana (Semana 1)")) {
        std::fprintf(stderr, "No se pudo inicializar el renderer. Saliendo.\n");
        return 1;
    }

    std::printf("Ventana abierta. Presiona ESC o cierra la ventana para salir.\n");

    bool running = true;
    while (running) {
        running = renderer.handleEvents();
        renderer.clear();
        // Semana 2: aqui se dibujaran los N cuerpos de la galaxia.
        renderer.present();
    }

    renderer.shutdown();
    std::printf("Ventana cerrada correctamente.\n");
    return 0;
}
