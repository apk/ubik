#!/bin/sh

cd "`dirname "$0"`"

echo "`date +%H:%M:%S` start"
exec /app/ubik /usr/bin/sleep 60 \
  --- /usr/bin/sleep 57 \
  ---5 test.sh
