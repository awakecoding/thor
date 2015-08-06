#!/bin/sh

png2y4m --no-dither -o test.in.y4m test.in.png
../thor enc -cf cfg.txt -if test.in.y4m -of test.thor -width 1024 -height 752
../thor dec test.thor test.out.yuv
yuv2yuv4mpeg test.out -w1024 -h752
y4m2png -o test.out.png test.out.y4m

rm test.in.y4m
rm test.out.yuv
rm test.out.y4m




