#!/usr/bin/env bash

git submodule init
git submodule update
VERSION=$(git describe --tags | sed 's/-.*//')
DIR=$(pwd)
sed -i "s/Version:.*/Version: ${VERSION}/" polymc.spec
sudo dnf builddep polymc.spec
rpmbuild -ba polymc.spec
