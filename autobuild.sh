#!/bin/bash

set -e

if [ ! -d `pwd`/build ];then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build && 
    cmake .. && 
    make

cd ..

if [ ! -d /usr/include/mymuduo ];then
    mkdir /usr/include/mymuduo
fi


#for herder in *.h;do
for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

cp `pwd`/noncopyable.hpp /usr/include/mymuduo

cp `pwd`/lib/libmymuduo.so /usr/lib