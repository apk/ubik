

# run-on-change ubik.c jfdt/.obj/jfdt.a jfdt/h/jfdt/*.h -- gcc -o ubik -I jfdt/h main.c jfdt/.obj/jfdt.a -- ./ubik /usr/bin/sleep 15 ---- /usr/bin/sleep 18 ----3 /usr/bin/echo adf ----2 /usr/bin/sleep 

test -d jfdt || git clone https://github.com/apk/jfdt.git || exit 7

sh jfdt/the.sh || exit 5

gcc -o ubik -I jfdt/h main.c jfdt/.obj/jfdt.a || exit 3
