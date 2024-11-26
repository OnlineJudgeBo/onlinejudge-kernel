#!/bin/bash
source /opt/nvm/nvm.sh

CONF=/home/judge/etc/judge.conf
export LANG=en_US.UTF-8
export HOME=/home/judge

trap "pkill judged; exit 0" SIGTERM SIGINT

case "${1:-''}" in
'start')
    echo "Starting judged..."
    /usr/bin/judged
    ;;
'stop')
    echo "Stopping judged..."
    pkill judged
    ;;
'status')
    ps aux | grep judged
    ;;
*)
    echo "Usage: $0 start|stop|status"
    exit 1
    ;;
esac

wait
