sh ../build.sh

d="`pwd`"
p="`dirname "$d"`"

docker run -t -v "$p":/app centos:7 /app/test-reaping/run.sh
