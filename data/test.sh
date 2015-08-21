#!/bin/sh

THOR=../thor

png2y4m --no-dither -o test.in.y4m test.in.png
$THOR enc -cf cfg.txt -if test.in.y4m -of test.thor
$THOR dec test.thor test.out.y4m
y4m2png -o test.out.png test.out.y4m

rm test.in.y4m
rm test.out.y4m




