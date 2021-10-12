#/bin/bash

inkscape -w 16 -h 16 -o logo_16.png logo.svg
inkscape -w 24 -h 24 -o logo_24.png logo.svg
inkscape -w 32 -h 32 -o logo_32.png logo.svg
inkscape -w 48 -h 48 -o logo_48.png logo.svg
inkscape -w 64 -h 64 -o logo_64.png logo.svg
inkscape -w 128 -h 128 -o logo_128.png logo.svg

convert logo_128.png logo_64.png logo_48.png logo_32.png logo_24.png logo_16.png Launcher.ico

inkscape -w 256 -h 256 -o logo_256.png logo.svg
inkscape -w 512 -h 512 -o logo_512.png logo.svg
inkscape -w 1024 -h 1024 -o logo_1024.png logo.svg

png2icns Launcher.icns logo_1024.png logo_512.png logo_256.png logo_128.png logo_32.png logo_16.png

rm -f logo_*.png
