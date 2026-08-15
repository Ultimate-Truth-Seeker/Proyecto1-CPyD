#ifndef ARGS_H
#define ARGS_H

#include <string>

// Configuracion de la simulacion, leida desde argumentos de linea de comando.
// Se pasa por referencia al resto de los modulos para evitar variables
// hard-coded en cualquier parte del programa (requisito de la rubrica).
struct SimConfig {
    int n = 0;                 // Cantidad de cuerpos (obligatorio, > 0)
    int width = 800;            // Ancho del canvas en pixeles
    int height = 600;            // Alto del canvas en pixeles
    double dt = 0.01;             // Paso de tiempo de la simulacion
    unsigned int seed = 0;         // Semilla pseudoaleatoria (0 = usar la hora actual)
    int threads = 0;                // Hilos OpenMP a usar (0 = automatico/omp_get_max_threads)
};

// Parsea argv y llena config con los valores correspondientes, aplicando
// valores por defecto para los argumentos no provistos. Hace programacion
// defensiva: valida tipos, rangos y la presencia del argumento obligatorio -n.
//
// Retorna true si el parseo y la validacion fueron exitosos.
// Retorna false si hubo un error, y en ese caso errorMsg describe el problema
// (para que main.cpp lo imprima y termine el programa de forma controlada).
//
// Argumentos soportados:
//   -n <int>        cantidad de cuerpos (OBLIGATORIO, debe ser > 0)
//   -w <int>         ancho del canvas (default 800, minimo 640 segun la guia)
//   -ht <int>         alto del canvas (default 600, minimo 480 segun la guia)
//   -dt <double>        paso de tiempo (default 0.01, debe ser > 0)
//   -seed <uint>          semilla pseudoaleatoria (default: hora actual del sistema)
//   -threads <int>           cantidad de hilos OpenMP (default 0 = automatico)
bool parseArgs(int argc, char** argv, SimConfig& config, std::string& errorMsg);

// Imprime a stdout la forma de uso del programa (se llama cuando el
// parseo falla o cuando el usuario pasa -h / --help).
void printUsage(const char* programName);

#endif // ARGS_H
