# Galaxy Sim — Screensaver Paralelo con OpenMP

Simulación de N-cuerpos con gravitación newtoniana, renderizada como un
screensaver de galaxia. Proyecto #1 de Computación Paralela y Distribuida.

## Dependencias

- g++ con soporte C++17 y OpenMP
- SDL2 (`libsdl2-dev`)
- pkg-config

En Ubuntu/Debian:
```bash
sudo apt-get install libsdl2-dev pkg-config
```

## Compilar

```bash
make              # compila las 3 pruebas de la Semana 1
make test_physics # solo la prueba de física + integrador (Persona A)
make test_renderer # solo la prueba de ventana SDL (Persona B)
make test_init    # solo la prueba de generación de galaxia (Persona C)
make clean        # borra los binarios compilados
```

## Correr las pruebas de Semana 1

```bash
./build/test_physics
./build/test_init
./build/test_renderer -n 500          # abre una ventana vacía; ESC para salir
./build/test_renderer -n 500 -w 1024 -ht 768 -threads 4
```

## Estructura del proyecto

Ver la sección "Estructura de archivos" en la planificación del proyecto
(`docs/` y el documento de planificación compartido con el equipo) para el
árbol completo y la descripción de cada módulo.

## Integrantes y módulos (Semana 1)

| Persona | Módulos |
|---|---|
| A | `body`, `physics_seq`, `integrator` |
| B | `args`, `renderer` |
| C | `init`, investigación de distribución de galaxia, diseño de formato CSV |
