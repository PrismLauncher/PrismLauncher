#/bin/bash

inkscape -w 16 -h 16 -o polymc_16.png org.polymc.PolyMC.svg
inkscape -w 24 -h 24 -o polymc_24.png org.polymc.PolyMC.svg
inkscape -w 32 -h 32 -o polymc_32.png org.polymc.PolyMC.svg
inkscape -w 48 -h 48 -o polymc_48.png org.polymc.PolyMC.svg
inkscape -w 64 -h 64 -o polymc_64.png org.polymc.PolyMC.svg
inkscape -w 128 -h 128 -o polymc_128.png org.polymc.PolyMC.svg

convert polymc_128.png polymc_64.png polymc_48.png polymc_32.png polymc_24.png polymc_16.png polymc.ico

inkscape -w 256 -h 256 -o polymc_256.png org.polymc.PolyMC.svg
inkscape -w 512 -h 512 -o polymc_512.png org.polymc.PolyMC.svg
inkscape -w 1024 -h 1024 -o polymc_1024.png org.polymc.PolyMC.svg

png2icns polymc.icns polymc_1024.png polymc_512.png polymc_256.png polymc_128.png polymc_32.png polymc_16.png

rm -f polymc_*.png
rm -rf polymc.iconset

for dir in ../launcher/resources/*/scalable
do
    cp -v org.polymc.PolyMC.svg $dir/launcher.svg
done
