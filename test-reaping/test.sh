#!/bin/sh
echo "`date +%H:%M:%S` up"
sleep 2
sleep 10 &
sleep 5 &
sleep 1
kill $!
sleep 2
sleep 11 &
a=$!
(sleep 8; kill $a; echo xxx) &
sleep 2
echo "`date +%H:%M:%S` down"
exit 5
