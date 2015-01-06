# Helper script for building fastd-android and its dependencies
# https://github.com/rlei/fastd-android
#!/bin/bash

echo "This script downloads and builds dependencies for fastd-android, as well as fastd-android itself."
echo "Make sure you have these packages installed:"
echo "  * Android NDK r10d or newer"
echo "  * for Debian/Ubuntu: sudo apt-get install curl build-eseentials automake bison cmake git libtool pkg-config"
echo "  * for Mac OS X: brew install automake libtool cmake bison"
echo "Hit ctrl-c now if you don't have all needed stuff yet."
read

SODIUM_VER=1.0.1
UECC_VER=4
LIBSODIUM_PATH=libsodium-${SODIUM_VER}
LIBUECC_PATH=libuecc-${UECC_VER}

if [ x$ANDROID_NDK_HOME == x ]; then
    echo "Set ANDROID_NDK_HOME first"; exit 1;
fi

if [ ! -d "build" ]; then
    mkdir build
fi

pushd build > /dev/null
WORK_DIR=`pwd`

if [ -d "${LIBSODIUM_PATH}" ]; then
    echo "It seems you already have libsodium downloaded.";
else
    echo "Downloading libsodium ${SODIUM_VER}..."
    curl -L https://github.com/jedisct1/libsodium/releases/download/${SODIUM_VER}/libsodium-${SODIUM_VER}.tar.gz | tar zxf - || exit 1
fi

pushd ${LIBSODIUM_PATH} > /dev/null
if [ ! -f "dist-build/android-armv7.sh" ]; then
    echo "Patching libsodium build scripts..."
    sed -i.bak 's/--enable-minimal//' dist-build/android-build.sh
    sed -e 's/-mthumb -marm -march=armv6/-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16/' dist-build/android-arm.sh > dist-build/android-armv7.sh
    chmod +x dist-build/android-armv7.sh
fi
if [ ! -d "libsodium-android-arm" ]; then
    dist-build/android-armv7.sh || exit 2
fi
if [ ! -d "libsodium-android-x86" ]; then
    dist-build/android-x86.sh || exit 2
fi
popd > /dev/null

if [ -d "android-cmake" ]; then
    echo "It seems you already have android-cmake downloaded.";
else
    echo "Downloading android-cmake"
    git clone https://github.com/taka-no-me/android-cmake.git;
fi
CMAKE_TOOLCHAIN=${WORK_DIR}/android-cmake/android.toolchain.cmake
ANDROID_CMAKE="cmake -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN}"
echo ">> android-cmake ready."

MAKE_TOOLCHAIN=${ANDROID_NDK_HOME}/build/tools/make-standalone-toolchain.sh
TOOLCHAIN_ARM=${WORK_DIR}/android-toolchain-arm
TOOLCHAIN_X86=${WORK_DIR}/android-toolchain-x86
if [ ! -d "${TOOLCHAIN_ARM}" ]; then
    $MAKE_TOOLCHAIN --platform=android-16 --arch=arm --install-dir=${TOOLCHAIN_ARM} || exit 3
fi
if [ ! -d "${TOOLCHAIN_X86}" ]; then
    $MAKE_TOOLCHAIN --platform=android-16 --arch=x86 --install-dir=${TOOLCHAIN_X86} || exit 3
fi
echo ">> android toolchains ready."

# from now on we won't use ANDROID_NDK_HOME to allow proper standalone toolchain detecting for libuecc
unset ANDROID_NDK_HOME

if [ -d "${LIBUECC_PATH}" ]; then
    echo "It seems you already have libuecc downloaded.";
else
    curl -k -L https://projects.universe-factory.net/attachments/download/71/libuecc-${UECC_VER}.tar.xz | tar Jxf - || exit 4
fi
if [ ! -d "libuecc-build-arm" ]; then
    mkdir libuecc-build-arm
    pushd libuecc-build-arm > /dev/null
    ANDROID_STANDALONE_TOOLCHAIN=${TOOLCHAIN_ARM} ${ANDROID_CMAKE} ../${LIBUECC_PATH} || exit 5
    make && make install || exit 6
    popd > /dev/null;
    echo ">> libuecc arm built."
fi
if [ ! -d "libuecc-build-x86" ]; then
    mkdir libuecc-build-x86
    pushd libuecc-build-x86 > /dev/null
    ANDROID_STANDALONE_TOOLCHAIN=${TOOLCHAIN_X86} ${ANDROID_CMAKE} ../${LIBUECC_PATH} || exit 5
    make && make install || exit 6
    popd > /dev/null;
    echo ">> libuecc x86 built."
fi

if [ ! -d "${TOOLCHAIN_ARM}/lib/pkgconfig" ]; then
    cp -a ${TOOLCHAIN_ARM}/user/lib/pkgconfig ${TOOLCHAIN_ARM}/lib/
    cp ${LIBSODIUM_PATH}/libsodium-android-arm/lib/pkgconfig/libsodium.pc ${TOOLCHAIN_ARM}/lib/pkgconfig/
    sed -i.bak 's/-L\${libdir} -lsodium/\${libdir}\/libsodium.a/' ${TOOLCHAIN_ARM}/lib/pkgconfig/libsodium.pc
    sed -i.bak 's/-L\${libdir} -luecc/\${libdir}\/libuecc.a/' ${TOOLCHAIN_ARM}/lib/pkgconfig/libuecc.pc
    echo ">> pkgconfig file patched for arm builds."
fi

if [ ! -d "${TOOLCHAIN_X86}/lib/pkgconfig" ]; then
    cp -a ${TOOLCHAIN_X86}/user/lib/pkgconfig ${TOOLCHAIN_X86}/lib/
    cp ${LIBSODIUM_PATH}/libsodium-android-arm/lib/pkgconfig/libsodium.pc ${TOOLCHAIN_X86}/lib/pkgconfig/
    sed -i.bak 's/-L\${libdir} -lsodium/\${libdir}\/libsodium.a/' ${TOOLCHAIN_X86}/lib/pkgconfig/libsodium.pc
    sed -i.bak 's/-L\${libdir} -luecc/\${libdir}\/libuecc.a/' ${TOOLCHAIN_X86}/lib/pkgconfig/libuecc.pc
    echo ">> pkgconfig file patched for x86 builds."
fi

HOMEBREW_BISON_PATH=`find /usr/local/Cellar/bison -name bin`
if [ x${HOMEBREW_BISON_PATH} != x ]; then
    USE_PATH=${HOMEBREW_BISON_PATH}:$PATH
else
    USE_PATH=$PATH
fi

COMMON_FASTD_BUILD_ARGS="-DWITH_CAPABILITIES=OFF -DWITH_STATUS_SOCKET=OFF -DENABLE_SYSTEMD=FALSE -DWITH_CIPHER_AES128_CTR=FALSE -DWITH_METHOD_XSALSA20_POLY1305=FALSE -DWITH_METHOD_GENERIC_POLY1305=FALSE -DWITH_CMDLINE_COMMANDS=FALSE"

if [ ! -d "fastd-build-arm" ]; then
    mkdir fastd-build-arm
fi

pushd fastd-build-arm > /dev/null
if [ ! -f "Makefile" ]; then
    PATH=${USE_PATH} ANDROID_STANDALONE_TOOLCHAIN=${TOOLCHAIN_ARM} PKG_CONFIG_LIBDIR=$ANDROID_STANDALONE_TOOLCHAIN/lib/pkgconfig ${ANDROID_CMAKE} ${COMMON_FASTD_BUILD_ARGS} -DWITH_CIPHER_SALSA2012_NACL=TRUE -DWITH_CIPHER_SALSA20_NACL=TRUE -DEXECUTABLE_OUTPUT_PATH=`pwd` ../.. || exit 7
fi

make && echo ">> fastd arm build ready in build/fastd-build-arm/"
popd > /dev/null

if [ ! -d "fastd-build-x86" ]; then
    mkdir fastd-build-x86
fi

pushd fastd-build-x86 /dev/null
if [ ! -f "Makefile" ]; then
    PATH=${USE_PATH} ANDROID_STANDALONE_TOOLCHAIN=${TOOLCHAIN_X86} PKG_CONFIG_LIBDIR=$ANDROID_STANDALONE_TOOLCHAIN/lib/pkgconfig ${ANDROID_CMAKE} ${COMMON_FASTD_BUILD_ARGS} -DWITH_CIPHER_SALSA2012_NACL=FALSE -DWITH_CIPHER_SALSA20_NACL=FALSE -DEXECUTABLE_OUTPUT_PATH=`pwd` ../.. || exit 7
fi

make && echo ">> fastd x86 build ready in build/fastd-build-x86/"
popd > /dev/null

popd > /dev/null

