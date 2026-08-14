# AI_CONTEXT.md — onlinejudge-kernel

## Qué es

Fork del kernel de HUSTOJ. Es el motor de evaluación de código del juez: no
expone ningún endpoint HTTP ni HTTP API — es un daemon en C/C++ que se
conecta directamente a MariaDB, compila y ejecuta las soluciones enviadas por
los usuarios, y escribe el resultado de vuelta en la base de datos. La API
(`onlinejudgebo-admin-api`) inserta la fila de `solution`; este kernel la
detecta, la juzga y actualiza esa misma fila. No hay llamada directa entre
la API y el kernel: la única interfaz es la tabla `solution` de MariaDB.

## Arquitectura interna

```
judged (daemon)
  -> cada N ms hace polling de trabajos pendientes en MariaDB
  -> por cada solución pendiente, lanza judge_client
      -> compila la solución con el toolchain del lenguaje
      -> ejecuta contra los casos de prueba con límites de tiempo/memoria
      -> opcionalmente corre anti_cheating.sh (Dolos) si aplica
      -> hace UPDATE del resultado en la tabla `solution`
```

- `judge/judged.cc` (452 líneas): el daemon. Hace polling de MariaDB y
  despacha `judge_client` por cada solución.
- `judge_client/judge_client.cc` (2348 líneas): compila/ejecuta una solución
  puntual y escribe el veredicto.
- `anti_cheating/anti_cheating.sh`: wrapper de Dolos (detección de plagio)
  invocado como paso adicional, no por `judged` directamente en el código
  visto — recibe `WORK_DIR SOLUTION_ID CONTEST_ID LANG PROBLEM_ID` y
  devuelve `mejorMatch,otroMatch,score` por stdout.
- `docker/entrypoint.sh` + `docker/supervisord.conf`: arrancan `judged` como
  proceso supervisado (usuario `judge`, reinicio automático, logs en
  `/var/log/supervisor/judged_*.log`).

## Contrato de interfaz (no HTTP): tabla `solution` en MariaDB

Es el único contrato real de este proyecto. Evidencia directa del código:

- **Selección de trabajos pendientes** (`judged.cc:163`):
  ```sql
  SELECT solution_id FROM solution
  WHERE language IN (<langs habilitados>) AND result < 2
  ORDER BY solution_id ASC LIMIT <max_running * 2>
  ```
  Es decir: `result < 2` es la condición de "pendiente de juzgar". Los
  lenguajes habilitados se leen de `judge.conf` (ver abajo).

- **Marcar en curso** (`judged.cc:273`, `_check_out_mysql`):
  ```sql
  UPDATE solution SET result=<code>, time=0, memory=0, judgetime=NOW()
  WHERE solution_id=<id> AND result<2 LIMIT 1
  ```

- **Escribir el veredicto final** (`judge_client.cc:493` y `:499`):
  ```sql
  UPDATE solution SET result=<code>, time=<ms>, memory=<kb>, pass_rate=<float>
  WHERE solution_id=<id> LIMIT 1
  -- o, sin pass_rate:
  UPDATE solution SET result=<code>, time=<ms>, memory=<kb>
  WHERE solution_id=<id> LIMIT 1
  ```

- **Códigos de resultado**: en su mayoría literales numéricos inline (estilo
  HUSTOJ clásico), no hay un enum centralizado. Solo dos están `#define`d en
  `judge_client/shared.h`: `OJ_RE=10` (Runtime Error), `OJ_CE=11` (Compile
  Error). El resto de los códigos (Accepted, Wrong Answer, TLE, MLE, etc.) se
  usan como números mágicos directamente en el código — hay que rastrearlos
  caso por caso si se necesita el mapeo completo; no están documentados en
  un solo lugar.

- **Configuración** (`judged.cc:43-45`): lee `/home/judge/etc/judge.conf` en
  texto plano (formato clave/valor simple parseado a mano) y usa
  `/home/judge/` como raíz de datos — de ahí sale `oj_lang_set` (lenguajes
  habilitados) y otros parámetros de runtime. `judge.conf` no está en este
  repo (se monta en runtime, vía `/judge-data` en el compose raíz).

- **Datos de contest/problema**: `anti_cheating.sh` espera encontrar el
  código fuente en `$WORK_DIR/data/contests/$CONTEST_ID/problem/$PROBLEM_ID/`
  — confirma que la estructura de archivos bajo `/home/judge` (o el volumen
  montado) sigue `data/contests/<id>/problem/<id>/`.

- **Sin conocimiento de cursos/academic**: este kernel solo conoce la tabla
  `solution` (juez clásico). Los conceptos de curso/asignación
  (`course_submission_context`, vistos en `onlinejudgebo-admin-api`) viven
  únicamente en la API — el kernel es agnóstico a ellos.

## Toolchains instalados (Dockerfile)

| Toolchain | Para qué |
| --- | --- |
| `build-essential`, `g++`, `make`, `flex` | Compilar C/C++ |
| Python 3.12 (compilado desde fuente) | Ejecutar soluciones Python3.12 |
| Adoptium JDK 8 (`temurin-8-jdk`) | Compilar/ejecutar Java |
| Node 20 vía NVM | Runtime para Dolos |
| `@dodona/dolos` (npm global) | Anti-plagio, invocado por `anti_cheating.sh` |
| `cJSON` (compilado desde fuente) | Librería C usada por `judge_client` |
| `pseint`/`psexport` (binarios en `pseint/`) | Soporte de PSeInt (pseudocódigo) |

El README declara soporte adicional para Python 2.7, 3.7 y C++x11 (el
Dockerfile solo instala 3.12 explícitamente; las demás versiones deben venir
de paquetes del sistema no listados aquí o de una imagen base distinta —
brecha, ver abajo).

## Variables de entorno / configuración

- `CONF=/home/judge/etc/judge.conf` — ruta del archivo de configuración del
  daemon (fijada por `ENV` en el Dockerfile, no configurable por env var).
- `HOME=/home/judge`.
- `LANG=en_US.UTF-8` (aunque el build genera locale `es_ES.UTF-8`).
- El proceso corre como usuario `judge` (uid 1536) vía supervisord, no como
  root, salvo el propio supervisord.
- Persistencia esperada: `/home/judge` (código fuente, datos de contest,
  logs) — en el `docker-compose.yml` raíz del workspace se monta
  `./storage/data/data:/judge-data` para `patito-api`; este kernel comparte
  esa misma data vía su propio volumen (ver `docker/docker-compose.yml`
  interno del repo, no inventariado en detalle aquí).

## Riesgos/brechas observadas

- Los códigos de veredicto (`result=<N>`) están hardcodeados como literales
  en múltiples puntos de `judge_client.cc` sin una tabla central — cualquier
  cambio de contrato con la API (que también interpreta estos códigos) debe
  revisarse con cuidado y a mano, no hay un único archivo que liste el enum
  completo.
- El polling de `judged.cc` es un `SELECT ... LIMIT` simple sin `FOR UPDATE`
  visible en el fragmento revisado; si hay más de un `judged` corriendo
  contra la misma base, dos procesos podrían tomar el mismo `solution_id`
  antes de que el primero ejecute el `UPDATE ... WHERE result<2`. El `UPDATE`
  final sí es atómico por fila, pero la ventana de selección no se auditó a
  fondo — si el despliegue corre un único `judged`, no es un problema
  práctico hoy.
- `judge.conf` no está versionado en este repo (se genera/monta en runtime),
  así que su formato exacto y todas las claves que `judged` espera no son
  verificables desde el código fuente sin ver ese archivo real.
- El soporte de Python 2.7/3.7/C++x11 mencionado en el README no tiene
  instalación explícita visible en el `Dockerfile` actual — posible
  documentación desactualizada o instalación implícita no revisada aquí.
- `anti_cheating.sh` tiene un fallback a `docker run ... dolos-cli` si el
  binario `dolos` no está en el PATH — eso implica Docker-in-Docker dentro
  del contenedor del kernel si el symlink de `dolos` falla, lo cual choca
  con el comentario del propio Dockerfile ("...para que `judged` pueda
  correr anti_cheating sin Docker-in-Docker"). Vale la pena confirmar cuál
  de las dos rutas se usa realmente en producción.
