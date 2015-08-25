#!/bin/sh

THOR=../thor

$THOR enc -cf cfg.txt -if test.in.png -of test.thor
$THOR dec test.thor test.out.png





