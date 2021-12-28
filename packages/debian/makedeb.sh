#!/bin/bash
mkdir builddir
cd builddir
cmake -DLauncher_LAYOUT=lin-system -DCMAKE_INSTALL_PREFIX=../polymc/usr ../../../
make -j$(nproc) install
cd ..
VERSION_PLACEHOLDER=$(git describe --tags | sed 's/-.*//')
cp polymc/DEBIAN/control.template polymc/DEBIAN/control
sed -i "2s/.*/Version: $VERSION_PLACEHOLDER/" polymc/DEBIAN/control
dpkg-deb --build polymc
