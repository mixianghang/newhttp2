#! /bin/sh
#mkdir -p $3/50mb_100mb
#mkdir -p $3/25mb_100mb
#mkdir -p $3/2500kb_100mb
#mkdir -p $3/250kb_100mb
#mkdir -p $3/25kb_100mb
mkdir -p $3/5kb_100mb
./asio-cl-1 http://156.56.83.24:6003 2500kb 2500kb $2 $2 $1 $3/5kb_100mb/ 2500000 2500000
#./asio-cl-1 http://156.56.83.24:6003 5kb 100mb $2 $2 $1 $3/5kb_100mb/ 5000 100000000
#./asio-cl-2 http://156.56.83.24:6003 5kb 100mb $2 $2 $1 $3/5kb_100mb/ 5000 100000000
#./asio-cl-1 http://156.56.83.24:6003 25kb 100mb $2 $2 $1 $3/25kb_100mb/ 25000 100000000
#./asio-cl-2 http://156.56.83.24:6003 25kb 100mb $2 $2 $1 $3/25kb_100mb/ 25000 100000000
#./asio-cl-1 http://156.56.83.24:6003 250kb 100mb $2 $2 $1 $3/250kb_100mb/ 250000 100000000
#./asio-cl-2 http://156.56.83.24:6003 250kb 100mb $2 $2 $1 $3/250kb_100mb/ 250000 100000000
#./asio-cl-1 http://156.56.83.24:6003 2500kb 100mb $2 $2 $1 $3/2500kb_100mb/ 250000 100000000
#./asio-cl-2 http://156.56.83.24:6003 2500kb 100mb $2 $2 $1 $3/2500kb_100mb/ 250000 100000000
#./asio-cl-1 http://156.56.83.24:6003 25mb 100mb $2 $2 $1 $3/25mb_100mb/ 250000 100000000
#./asio-cl-2 http://156.56.83.24:6003 25mb 100mb $2 $2 $1 $3/25mb_100mb/ 250000 100000000
#./asio-cl-1 http://156.56.83.24:6003 50mb 100mb $2 $2 $1 $3/50mb_100mb/ 250000 100000000
#./asio-cl-2 http://156.56.83.24:6003 50mb 100mb $2 $2 $1 $3/50mb_100mb/ 250000 100000000
#
