# Thor Video Codec 

Implementation of [https://tools.ietf.org/html/draft-fuldseth-netvc-thor](https://tools.ietf.org/html/draft-fuldseth-netvc-thor)

The experimental Torn development branch is intended to use Thor as a real-time screencasting codec.

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

thor enc -cf cfg.txt -if ducks.yuv -of ducks.thor -width 1280 -height 720

The optional configuration file contains one command-line argument per line, with the ';' character used for comments.

decoder:

thor dec ducks.thor ducks.y4m

Supported formats: .y4m, .yuv, .bmp, .png, .rgb

