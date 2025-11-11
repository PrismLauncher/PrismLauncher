#!/usr/bin/env zsh

# Run this script from the directory containing "PrismLauncher.app"

CODE_SIGN_IDENTITY="${1:--}"
MAIN_ENTITLEMENTS_FILE="${2:-../program_info/App.entitlements}"

################ FRAMEWORKS ################
cd "PrismLauncher.app/Contents/Frameworks" || exit 1
# See https://sparkle-project.org/documentation/sandboxing/
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" -o runtime Sparkle.framework/Versions/B/XPCServices/Installer.xpc
# For Sparkle versions >= 2.6
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" -o runtime --preserve-metadata=entitlements Sparkle.framework/Versions/B/XPCServices/Downloader.xpc
# For Sparkle versions < 2.6
#codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" -o runtime --entitlements Entitlements/Downloader.entitlements Sparkle.framework/Versions/B/XPCServices/Downloader.xpc
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" -o runtime Sparkle.framework/Versions/B/Autoupdate
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" -o runtime Sparkle.framework/Versions/B/Updater.app

codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" ./*.framework
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" ./*.dylib

################ XPC SERVICES ################
if cd "../XPCServices"; then
    codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" -o runtime ./*.xpc
fi

################ PLUGINS ################
cd "../PlugIns" || exit 1
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" iconengines/*.dylib
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" imageformats/*.dylib
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" networkinformation/*.dylib
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" platforminputcontexts/*.dylib
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" platforms/*.dylib
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" styles/*.dylib
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" tls/*.dylib
cd "../MacOS" || exit 1
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" jars/*.jar
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" *.dylib

################ APP ################
cd "../../.." || exit 1
codesign -f --timestamp -s "$CODE_SIGN_IDENTITY" --entitlements "$MAIN_ENTITLEMENTS_FILE" -o runtime ./PrismLauncher.app/Contents/MacOS/prismlauncher