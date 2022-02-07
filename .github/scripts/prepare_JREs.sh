#!/usr/bin/env bash

URL_JDK8="https://api.adoptium.net/v3/binary/latest/8/ga/linux/x64/jre/hotspot/normal/eclipse"
URL_JDK17="https://api.adoptium.net/v3/binary/latest/17/ga/linux/x64/jre/hotspot/normal/eclipse"

mkdir -p JREs
pushd JREs

wget --content-disposition "$URL_JDK8"
wget --content-disposition "$URL_JDK17"

for file in *;
do
    mkdir temp

    # Handle OpenJDK17 archive
    re='(OpenJDK17U-jre_x64_linux_hotspot_17.(.*).tar.gz)'
    if [[ $file =~ $re ]];
    then
        version=${BASH_REMATCH[2]}
        version_edit=$(echo $version | sed -e 's/_/+/g')
        dir_name=jdk-17.$version_edit-jre
        echo $dir_name
        mkdir jre17
        tar -xzf $file -C temp
        pushd temp/$dir_name
        cp -r . ../../jre17
        popd

    fi

    # Handle OpenJDK8 archive
    re='(OpenJDK8U-jre_x64_linux_hotspot_8(.*).tar.gz)'
    if [[ $file =~ $re ]];
    then
        version=${BASH_REMATCH[2]}
        version_edit=$(echo $version | sed -e 's/b/-b/g')
        dir_name=jdk8$version_edit-jre
        mkdir jre8
        tar -xzf $file -C temp
        pushd temp/$dir_name
        cp -r . ../../jre8
        popd
    fi

    rm -rf temp
done

popd
