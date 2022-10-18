#/bin/bash

# ICO

inkscape -w 16 -h 16 -o prismlauncher_16.png org.prismlauncher.PrismLauncher.svg
inkscape -w 24 -h 24 -o prismlauncher_24.png org.prismlauncher.PrismLauncher.svg
inkscape -w 32 -h 32 -o prismlauncher_32.png org.prismlauncher.PrismLauncher.svg
inkscape -w 48 -h 48 -o prismlauncher_48.png org.prismlauncher.PrismLauncher.svg
inkscape -w 64 -h 64 -o prismlauncher_64.png org.prismlauncher.PrismLauncher.svg
inkscape -w 128 -h 128 -o prismlauncher_128.png org.prismlauncher.PrismLauncher.svg

convert prismlauncher_128.png prismlauncher_64.png prismlauncher_48.png prismlauncher_32.png prismlauncher_24.png prismlauncher_16.png prismlauncher.ico

rm -f prismlauncher_*.png

inkscape -w 1024 -h 1024 -o prismlauncher_1024.png org.prismlauncher.PrismLauncher.bigsur.svg

mkdir prismlauncher.iconset

sips -z 16 16     prismlauncher_1024.png --out prismlauncher.iconset/icon_16x16.png
sips -z 32 32     prismlauncher_1024.png --out prismlauncher.iconset/icon_16x16@2x.png
sips -z 32 32     prismlauncher_1024.png --out prismlauncher.iconset/icon_32x32.png
sips -z 64 64     prismlauncher_1024.png --out prismlauncher.iconset/icon_32x32@2x.png
sips -z 128 128   prismlauncher_1024.png --out prismlauncher.iconset/icon_128x128.png
sips -z 256 256   prismlauncher_1024.png --out prismlauncher.iconset/icon_128x128@2x.png
sips -z 256 256   prismlauncher_1024.png --out prismlauncher.iconset/icon_256x256.png
sips -z 512 512   prismlauncher_1024.png --out prismlauncher.iconset/icon_256x256@2x.png
sips -z 512 512   prismlauncher_1024.png --out prismlauncher.iconset/icon_512x512.png
cp prismlauncher_1024.png prismlauncher.iconset/icon_512x512@2x.png

iconutil -c icns prismlauncher.iconset

rm -f prismlauncher_*.png
rm -rf prismlauncher.iconset

for dir in ../launcher/resources/*/scalable
do
    cp -v org.prismlauncher.PrismLauncher.svg $dir/launcher.svg
done
