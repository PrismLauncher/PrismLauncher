#!/bin/bash

INSTDIR=$XDG_DATA_HOME/multimc
if [ `getconf LONG_BIT` = "64" ]
then
    PACKAGE="mmc-stable-lin64.tar.gz"
else
    PACKAGE="mmc-stable-lin32.tar.gz"
fi

deploy() {
    mkdir -p $INSTDIR
    cd ${INSTDIR}

    wget --progress=dot:force "https://files.multimc.org/downloads/${PACKAGE}" 2>&1 | sed -u 's/.* \([0-9]\+%\)\ \+\([0-9.]\+.\) \(.*\)/\1\n# Downloading at \2\/s, ETA \3/' | zenity --progress --auto-close --auto-kill --title="Downloading MultiMC..."

    tar -xzf ${PACKAGE} --transform='s,MultiMC/,,'
    rm ${PACKAGE}
    chmod +x MultiMC
}

runmmc() {
    cd ${INSTDIR}
    ./MultiMC
}

if [[ ! -f ${INSTDIR}/MultiMC ]]; then
    deploy
    runmmc
else
    runmmc
fi
