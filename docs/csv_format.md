# Diseño del formato CSV para la bitácora de mediciones (Persona C — Semana 1)

Este formato lo va a usar `benchmark.cpp` (semana 2) para exportar los
resultados que alimentan el Anexo 3 (bitácora de pruebas, mínimo 10
mediciones) y las gráficas de speedup/eficiencia del informe.

## Archivo: `tests/bitacora/benchmark_results.csv`

| Columna         | Tipo   | Descripción                                                        |
|-----------------|--------|---------------------------------------------------------------------|
| `run_id`        | int    | Identificador incremental de la corrida                             |
| `n_bodies`      | int    | Cantidad de cuerpos simulados (parámetro N)                         |
| `mode`          | string | `"sequential"` o `"parallel"`                                       |
| `num_threads`   | int    | Hilos OpenMP usados (1 si `mode=sequential`)                        |
| `frame`         | int    | Número de frame dentro de la corrida (0-indexado)                   |
| `force_time_ms` | double | Tiempo de `computeForces*` en ese frame, en milisegundos             |
| `integrate_time_ms` | double | Tiempo de `integrateEuler` en ese frame, en milisegundos       |
| `total_frame_time_ms` | double | Tiempo total del frame (fuerzas + integración + render)     |
| `fps`           | double | FPS instantáneo calculado en ese frame                                |

### Ejemplo de filas

```csv
run_id,n_bodies,mode,num_threads,frame,force_time_ms,integrate_time_ms,total_frame_time_ms,fps
1,500,sequential,1,0,12.4,0.8,14.1,70.9
1,500,sequential,1,1,12.1,0.7,13.8,72.4
2,500,parallel,4,0,3.9,0.8,5.5,181.8
2,500,parallel,4,1,3.7,0.7,5.2,192.3
```

## Cómo se usa para el Anexo 3

- Para cada `n_bodies` de interés (ej. 100, 500, 1000, 5000, 10000), se corre
  el modo secuencial y el paralelo (con 2, 4 y 8 hilos), guardando cada
  combinación como un `run_id` distinto.
- El **speedup** se calcula como `tiempo_secuencial_promedio / tiempo_paralelo_promedio`
  agrupando por `n_bodies` y `num_threads` (promediando `total_frame_time_ms`
  entre los frames de cada corrida).
- La **eficiencia** se calcula como `speedup / num_threads`.
- Esto da directamente el mínimo de 10 mediciones que pide el Anexo 3 (5
  valores de N × 2 modos, sin contar las variantes de número de hilos) y
  permite graficar speedup/eficiencia vs. N y vs. cantidad de hilos.

## Nota de implementación (para cuando Persona C escriba `benchmark.cpp`)

- Abrir el archivo en modo *append* (`std::ofstream::app`) para no perder
  corridas anteriores entre ejecuciones del programa.
- Escribir el encabezado solo si el archivo no existe todavía.
- `scripts/run_benchmarks.sh` se encargará de invocar el binario con
  distintas combinaciones de `-n` y `-threads`, y de correr el modo
  secuencial (`-threads 1`, sin sección paralela) como baseline.
