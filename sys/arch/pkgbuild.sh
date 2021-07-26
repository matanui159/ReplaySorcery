#!/usr/bin/env bash

VERSION=$1

if [ ! "${VERSION?}" ]; then
   echo 'Usage: create-pkgbuild.sh {version}'
   exit 1
fi

cat <<- EOF
pkgname=replay-sorcery
pkgver=$VERSION
pkgrel=1
pkgdesc='An open-source, instant-replay screen recorder for Linux'
arch=(i686 x86_64)
license=(GPL3)
depends=(gcc-libs ffmpeg libx11 libxcb libpulse libdrm)
makedepends=(cmake git)
url='https://github.com/matanui159/ReplaySorcery'
source=("\${pkgname}"::git+"\${url}".git#tag="\${pkgver}"
        git+https://github.com/ianlancetaylor/libbacktrace.git)
sha256sums=('SKIP'
            'SKIP')

prepare() {
   cd \${pkgname}
   git submodule init
   git config submodule."dep/libbacktrace".url ../libbacktrace
   git submodule update
}

build() {
   cmake -B build -S "\${pkgname}" \\
      -DCMAKE_BUILD_TYPE=Release \\
      -DCMAKE_INSTALL_PREFIX=/usr
   cmake --build build
}

package() {
   DESTDIR="\${pkgdir}" cmake --install build
}
EOF
