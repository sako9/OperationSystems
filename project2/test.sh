#!/bin/sh
# This is a comment

rm *.txt
make clean
make
echo
echo Testing will take around 6 seconds...
./run | sort > student_out.txt

if diff -w student_out.txt correct_output/correct_output.txt; then
    echo Success-----------------------------------------------Success
else
    echo Fail--------------------------------------------------Fail
fi
