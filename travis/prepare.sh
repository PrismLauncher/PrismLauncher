set -e

if [ "$TRAVIS_OS_NAME" = "linux" ]
then
	QT_WITHOUT_DOTS=qt$(echo $QT_VERSION | grep -oP "[^\.]*" | tr -d '\n' | tr '[:upper:]' '[:lower]')
	QT_PKG_PREFIX=$(echo $QT_WITHOUT_DOTS | cut -c1-4)
	QT_PKG_INSTALL=$QT_PKG_PREFIX
	if [ "$QT_PKG_PREFIX" = "qt50" ]; then QT_PKG_PREFIX=qt QT_PKG_INSTALL=qt5; fi
	echo $QT_WITHOUT_DOTS
	echo $QT_PKG_PREFIX
	echo $QT_PKG_INSTALL
	if [ "$TRAVIS_DIST" = "precise" ]; then
		sudo add-apt-repository -y ppa:beineri/opt-${QT_WITHOUT_DOTS}
	else
		sudo add-apt-repository -y ppa:beineri/opt-${QT_WITHOUT_DOTS}-$TRAVIS_DIST
	fi
	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test # for a recent GCC
	sudo add-apt-repository "deb http://llvm.org/apt/${TRAVIS_DIST}/ llvm-toolchain-${TRAVIS_DIST}-3.5 main"

	sudo apt-get update -qq
	sudo apt-get install ${QT_PKG_PREFIX}base ${QT_PKG_PREFIX}svg ${QT_PKG_PREFIX}tools

	sudo mkdir -p /opt/cmake-3/
	wget --no-check-certificate http://www.cmake.org/files/v3.9/cmake-3.9.3-Linux-x86_64.sh
	sudo sh cmake-3.9.3-Linux-x86_64.sh --skip-license --prefix=/opt/cmake-3/

	export CMAKE_PREFIX_PATH=/opt/$QT_PKG_INSTALL/lib/cmake
	export PATH=/opt/cmake-3/bin:/opt/$QT_PKG_INSTALL/bin:$PATH

	if [ "$CXX" = "g++" ]; then
		sudo apt-get install -y -qq g++-5
		export CXX='g++-5' CC='gcc-5'
	fi
	if [ "$CXX" = "clang++" ]; then
		wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key|sudo apt-key add -
		sudo apt-get install -y -qq clang-3.5 liblldb-3.5 libclang1-3.5 libllvm3.5 lldb-3.5 llvm-3.5 llvm-3.5-runtime
		export CXX='clang++-3.5' CC='clang-3.5'
	fi
else
	brew update
	brew install qt5
	brew install cmake
	export CMAKE_PREFIX_PATH=/usr/local/lib/cmake
fi

# Output versions
cmake -version
qmake -version
$CXX -v
echo "CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH"
