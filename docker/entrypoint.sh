#!/bin/bash
set -e
id
echo "Iniciando el servicio 'judged'"
exec /usr/bin/judged /home/judge
