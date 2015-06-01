set -e

if [ "$TRAVIS_OS_NAME" = "linux" ]
then
	QT_WITHOUT_DOTS=qt$(echo $QT_VERSION | grep -oP "[^\.]*" | tr -d '\n' | tr '[:upper:]' '[:lower]')
	QT_PKG_PREFIX=$(echo $QT_WITHOUT_DOTS | cut -c1-4)
	echo $QT_WITHOUT_DOTS
	echo $QT_PKG_PREFIX
	sudo add-apt-repository -y ppa:beineri/opt-${QT_WITHOUT_DOTS}
	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test # for a recent GCC
	sudo add-apt-repository "deb http://llvm.org/apt/precise/ llvm-toolchain-precise-3.5 main"

	sudo apt-get update -qq
	sudo apt-get install ${QT_PKG_PREFIX}base ${QT_PKG_PREFIX}svg ${QT_PKG_PREFIX}tools ${QT_PKG_PREFIX}x11extras ${QT_PKG_PREFIX}webkit

	sudo mkdir -p /opt/cmake-3/
	wget http://www.cmake.org/files/v3.2/cmake-3.2.2-Linux-x86_64.sh
	sudo sh cmake-3.2.2-Linux-x86_64.sh --skip-license --prefix=/opt/cmake-3/

	export CMAKE_PREFIX_PATH=/opt/$QT_PKG_PREFIX/lib/cmake
	export PATH=/opt/cmake-3/bin:/opt/$QT_PKG_PREFIX/bin:$PATH

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
