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

    re='(OpenJDK([[:digit:]]+)U-jre_x64_linux_hotspot_([[:digit:]]+)(.*).tar.gz)'
    if [[ $file =~ $re ]];
    then
        version_major=${BASH_REMATCH[2]}
        version_trailing=${BASH_REMATCH[4]}

        if [ $version_major = 17 ];
        then
            hyphen='-'
        else
            hyphen=''
        fi

        version_edit=$(echo $version_trailing | sed -e 's/_/+/g' | sed -e 's/b/-b/g')
        dir_name=jdk$hyphen$version_major$version_edit-jre
        mkdir jre$version_major
        tar -xzf $file -C temp
        pushd temp/$dir_name
        cp -r . ../../jre$version_major
        popd
    fi

    rm -rf temp
done

popd
