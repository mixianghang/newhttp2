#! /bin/sh
case "$1" in
 "one")
 ./test $2 "./asio-cl-1 http://156.56.83.24:6003 $3 $4" "one-conn-$3-$5.csv" "one-conn-$4-$5.csv"
 ;;
 "two")
 ./test $2 "./asio-cl-2 http://156.56.83.24:6003 $3 $4" "two-conn-$3-$5.csv" "two-conn-$4-$5.csv"
 ;;
esac

