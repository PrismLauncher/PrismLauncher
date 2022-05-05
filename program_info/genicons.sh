#/bin/bash

# ICO

inkscape -w 16 -h 16 -o polymc_16.png org.polymc.PolyMC.svg
inkscape -w 24 -h 24 -o polymc_24.png org.polymc.PolyMC.svg
inkscape -w 32 -h 32 -o polymc_32.png org.polymc.PolyMC.svg
inkscape -w 48 -h 48 -o polymc_48.png org.polymc.PolyMC.svg
inkscape -w 64 -h 64 -o polymc_64.png org.polymc.PolyMC.svg
inkscape -w 128 -h 128 -o polymc_128.png org.polymc.PolyMC.svg

convert polymc_128.png polymc_64.png polymc_48.png polymc_32.png polymc_24.png polymc_16.png polymc.ico

rm -f polymc_*.png

inkscape -w 1024 -h 1024 -o polymc_1024.png org.polymc.PolyMC.bigsur.svg

mkdir polymc.iconset

sips -z 16 16     polymc_1024.png --out polymc.iconset/icon_16x16.png
sips -z 32 32     polymc_1024.png --out polymc.iconset/icon_16x16@2x.png
sips -z 32 32     polymc_1024.png --out polymc.iconset/icon_32x32.png
sips -z 64 64     polymc_1024.png --out polymc.iconset/icon_32x32@2x.png
sips -z 128 128   polymc_1024.png --out polymc.iconset/icon_128x128.png
sips -z 256 256   polymc_1024.png --out polymc.iconset/icon_128x128@2x.png
sips -z 256 256   polymc_1024.png --out polymc.iconset/icon_256x256.png
sips -z 512 512   polymc_1024.png --out polymc.iconset/icon_256x256@2x.png
sips -z 512 512   polymc_1024.png --out polymc.iconset/icon_512x512.png
cp polymc_1024.png polymc.iconset/icon_512x512@2x.png

iconutil -c icns polymc.iconset

rm -f polymc_*.png
rm -rf polymc.iconset

for dir in ../launcher/resources/*/scalable
do
    cp -v org.polymc.PolyMC.svg $dir/launcher.svg
done
