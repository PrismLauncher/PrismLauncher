#!/bin/bash

INSTDIR=~/MultiMC

deploy() {
    mkdir -p $INSTDIR
    cd ${INSTDIR}

    wget --progress=dot:force "http://files.multimc.org/downloads/mmc-stable-lin64.tar.gz" 2>&1 | sed -u 's/.* \([0-9]\+%\)\ \+\([0-9.]\+.\) \(.*\)/\1\n# Downloading at \2\/s, ETA \3/' | zenity --progress --auto-close --auto-kill --title="Downloading MultiMC..."

    tar -xzf mmc-stable-lin64.tar.gz --strip-components=2
    rm mmc-stable-lin64.tar.gz
    chmod +x MultiMC
    ./MultiMC
}

runmmc() {
    cd ${INSTDIR}
    ./MultiMC
}

if [[ ! -f ${INSTDIR}/MultiMC ]]; then
    deploy
else
    runmmc
fi
