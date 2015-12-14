#!/bin/sh
# This is a comment

rm *.out
make clean
make
echo
./spksp commandFile.txt 1

sort output.out -o student_out1.out

if diff -w student_out1.out correct_output/correct_output.txt; then
    echo Success-----------------------------------------------Success
else
    echo Fail--------------------------------------------------Fail
fi
rm *.out

./spksp commandFile.txt 10

sort output.out -o student_out10.out

if diff -w student_out10.out correct_output/correct_output.txt; then
    echo Success-----------------------------------------------Success
else
    echo Fail--------------------------------------------------Fail
fi
rm *.out

./spksp commandFile.txt 100

sort output.out -o student_out100.out

if diff -w student_out100.out correct_output/correct_output.txt; then
    echo Success-----------------------------------------------Success
else
    echo Fail--------------------------------------------------Fail
fi
rm *.out

./spksp commandFile.txt 1000

sort output.out -o student_out1000.out

if diff -w student_out1000.out correct_output/correct_output.txt; then
    echo Success-----------------------------------------------Success
else
    echo Fail--------------------------------------------------Fail
fi
rm *.out
