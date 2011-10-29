#!/bin/bash

set -e
set -x

clean()
{
    rm -rf ${AVG_PATH}/bin/
    rm -rf ${AVG_PATH}/lib/
    sudo rm -rf ${AVG_PATH}/include/

    mkdir ${AVG_PATH}/bin
    mkdir ${AVG_PATH}/lib
    mkdir ${AVG_PATH}/include
}

buildLib()
{
    LIBNAME=$1
    CONFIG_ARGS=$2

    cd ${LIBNAME}
    ./configure --prefix=${AVG_PATH} ${CONFIG_ARGS}
    make clean
    make -j5
    make install
    cd ..
}

buildlibjpeg()
{
    cd jpeg-6b
    cp ${AVG_PATH}/share/libtool/config/config.sub .
    cp ${AVG_PATH}/share/libtool/config/config.guess .
    ./configure --prefix=${AVG_PATH}
    make clean
    make -j5
    make install-lib
    make install-headers
    ranlib ../../lib/libjpeg.a
    cd ..
}

buildlibpng()
{
    cd libpng-1.2.41
    ./configure --prefix=${AVG_PATH} --disable-shared
    make clean
    make -j5
    make install
    cd ..
}

buildglib()
{
    cd glib-2.29.2
    LDFLAGS="-framework ApplicationServices $LDFLAGS -lresolv" ./configure  --prefix=${AVG_PATH} --disable-shared --enable-static 
    make clean
    make -j5
    make install
    cd ..
}

buildfontconfig()
{
    cd fontconfig-2.7.0
    automake
    LDFLAGS="-framework ApplicationServices ${LDFLAGS}" ./configure --prefix=${AVG_PATH} --disable-shared --with-add-fonts=/Library/Fonts,/System/Library/Fonts,~/Library/Fonts --with-confdir=/etc/fonts --with-cache-dir=~/.fontconfig --with-cache-dir=~/.fontconfig
    make clean
    make -j5
    sudo make install
    sudo chown -R `whoami` ~/.fontconfig
    cd ..    
}

buildgdkpixbuf()
{
    cd gdk-pixbuf-2.23.3
    LDFLAGS="-framework ApplicationServices $LDFLAGS -lresolv" ./configure --prefix=${AVG_PATH} --disable-shared --with-included-loaders
    make clean
    make -j5
    make install
    cd ..
}

buildboost()
{
    cd boost_1_41_0
    ./bootstrap.sh --prefix=${AVG_PATH} --with-libraries=python,thread
    ./bjam clean
    ./bjam install
    cd ..
    rm -f ../lib/libboost_thread.dylib
    rm -f ../lib/libboost_python.dylib
}
if [[ x"${AVG_PATH}" == "x" ]]
then
    echo ${AVG_PATH}
    echo Please set AVG_PATH and call 'source mac/avg_env.sh' before calling this script.
    exit -1 
fi

if [[ x"${PKG_CONFIG_PATH}" == "x" ]]
then
    echo Please call 'source mac/avg_env.sh' before calling this script.
    exit -1 
fi

clean

cd ../deps

DARWINVER=`uname -r`
DARWINMAJORVER=${DARWINVER%%.*}

buildLib libtool-2.2.6
buildLib autoconf-2.63
buildLib automake-1.11
buildlibjpeg
buildLib tiff-3.8.2 --disable-shared 
buildlibpng
buildLib libxml2-2.6.32 --disable-shared
buildLib pkg-config-0.20
if [[ "${DARWINMAJORVER}" == "10" ]]
then
    buildLib ffmpeg "--arch=x86_64 --disable-debug --enable-pthreads --disable-ffserver --disable-muxer=matroska --disable-demuxer=matroska --disable-muxer=matroska_audio --disable-decoder=vc1 --disable-decoder=wmv3"
else
    buildLib ffmpeg "--disable-debug --enable-pthreads --disable-ffserver --disable-muxer=matroska --disable-demuxer=matroska --disable-muxer=matroska_audio"
fi

buildLib SDL-1.2.14 "--disable-shared --disable-cdrom --disable-threads --disable-file --disable-video-x11 --without-x"
buildLib gettext-0.18.1.1 "--disable-shared --with-included-gettext --disable-csharp  --disable-libasprintf"
buildglib

buildLib freetype-2.4.4 "--disable-shared --with-old-mac-fonts"
buildLib expat-2.0.0 --disable-shared

buildfontconfig

buildLib pixman-0.22.0 --disable-shared
buildLib cairo-1.10.2 "--disable-shared --enable-xlib=no --enable-xlib-xrender=no --enable-quartz=no --enable-quartz-font=no --enable-quartz-image=no --enable-ps=no --enable-pdf=no --enable-svg=no"
buildLib pango-1.24.4 "--disable-shared --without-x --with-included-modules=yes"

buildgdkpixbuf
buildLib librsvg-2.34.0 --disable-shared

buildboost

buildLib libdc1394-2.0.2 "--disable-shared --disable-doxygen-doc --without-x"

cd ../libavg
