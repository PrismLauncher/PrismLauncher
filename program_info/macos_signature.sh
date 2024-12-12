#!/usr/bin/env zsh

# Run this script from the directory containing "PrismLauncher.app"

CODE_SIGN_IDENTITY="${1:--}"
MAIN_ENTITLEMENTS_FILE="${2:-../program_info/App.entitlements}"

if [[ "$CODE_SIGN_IDENTITY" == "Developer ID Application"* ]]; then
    CODE_SIGN_IDENTITY=("$CODE_SIGN_IDENTITY" --timestamp)
fi

################ FRAMEWORKS ################
cd "PrismLauncher.app/Contents/Frameworks" || exit 1
# See https://sparkle-project.org/documentation/sandboxing/
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" -o runtime Sparkle.framework/Versions/B/XPCServices/Installer.xpc
# For Sparkle versions >= 2.6
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" -o runtime --preserve-metadata=entitlements Sparkle.framework/Versions/B/XPCServices/Downloader.xpc
# For Sparkle versions < 2.6
#codesign -f -s "${CODE_SIGN_IDENTITY[@]}" -o runtime --entitlements Entitlements/Downloader.entitlements Sparkle.framework/Versions/B/XPCServices/Downloader.xpc
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" -o runtime Sparkle.framework/Versions/B/Autoupdate
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" -o runtime Sparkle.framework/Versions/B/Updater.app

codesign -f -s "${CODE_SIGN_IDENTITY[@]}" ./*.framework
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" ./*.dylib

################ XPC SERVICES ################
if cd "../XPCServices"; then
    codesign -f -s "${CODE_SIGN_IDENTITY[@]}" -o runtime ./*.xpc
fi

################ PLUGINS ################
cd "../MacOS" || exit 1
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" iconengines/*.dylib
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" imageformats/*.dylib
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" platforms/*.dylib
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" jars/*.jar
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" styles/*.dylib
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" tls/*.dylib

################ APP ################
cd "../../.." || exit 1
codesign -f -s "${CODE_SIGN_IDENTITY[@]}" --entitlements "$MAIN_ENTITLEMENTS_FILE" -o runtime ./PrismLauncher.app