#!/bin/sh

## If current working dirctory is ./scripts, ask to invoke from one directory up
if [ ! -d "scripts" ]; then
	echo "Please run this script from the root directory of the project"
	exit 1
fi

find . -type f -name '*.png' -not -path '*/libraries/*' -exec oxipng --opt max --strip all --alpha --interlace 0 {} \;
