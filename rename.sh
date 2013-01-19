#!/bin/sh

dir=$1
cd $dir
echo "In `pwd`"
for file in `ls *.cpp`
do
	base=`basename $file .cpp`
	mv $file $base.cc
	echo "$file --> $base.cc" 
done
