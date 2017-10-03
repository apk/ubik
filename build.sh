

cd "`dirname "$0"`"

# run-on-change ubik.c jfdt/.obj/jfdt.a jfdt/h/jfdt/*.h -- gcc -o ubik -I jfdt/h main.c jfdt/.obj/jfdt.a -- ./ubik /usr/bin/sleep 15 ---- /usr/bin/sleep 18 ----3 /usr/bin/echo adf ----2 /usr/bin/sleep 

jfdt=jfdt
if test -d "$jfdt"; then
    : nix
elif test -d ../jfdt; then
    jfdt=../jfdt
else
    git clone https://github.com/apk/jfdt.git || exit 7
fi

set -x

sh "$jfdt"/the.sh || exit 5

gcc -o ubik -I "$jfdt"/h main.c "$jfdt"/.obj/jfdt.a || exit 3
