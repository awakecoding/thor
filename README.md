# Thor Video Codec 

Implementation of [https://tools.ietf.org/html/draft-fuldseth-netvc-thor](https://tools.ietf.org/html/draft-fuldseth-netvc-thor)

## Build

Linux:

cmake .
cmake -G "Eclipse CDT4 - Unix Makefiles" .

OS X:

cmake .
cmake -G Xcode .

Windows:

cmake -G "Visual Studio 12" .
cmake -G "Visual Studio 12 Win64" .

cmake -G "Visual Studio 12" -T "Intel C++ Compiler XE 15" .
cmake -G "Visual Studio 12 Win64" -T "Intel C++ Compiler XE 15" .

## Usage

See [https://wiki.xiph.org/Daala_Quickstart](Daala Quickstart) for test media.

encoder:

thor enc -cf cfg.txt -if ducks.y4m -of ducks.thor
thor enc -cf cfg.txt -if ducks.png -of ducks.thor
thor enc -cf cfg.txt -if ducks.bmp -of ducks.thor

decoder:

thor dec ducks.thor ducks.y4m
thor dec ducks.thor ducks.png
thor dec ducks.thor ducks.bmp
