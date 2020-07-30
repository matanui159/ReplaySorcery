#!/usr/bin/env bash

VERSION=$1

if [ ! "${VERSION?}" ]; then
   echo "Usage: create-pkgbuild.sh {version}"
   exit 1
fi

cat <<- EOF
pkgname=replay-sorcery
pkgver=$VERSION
pkgrel=1
pkgdesc="An open-source, instant-replay screen recorder for Linux"
arch=("i686" "x86_64")
license=("GPL3")
depends=("gcc-libs" "libxext")
makedepends=("cmake" "make" "git" "nasm")
url="https://github.com/matanui159/ReplaySorcery"
source=("\${pkgname}::git+\${url}.git#tag=\${pkgver}")
sha256sums=("SKIP")

prepare() {
   mkdir -p "\${pkgname}/build"
   git -C "\${pkgname}" submodule update --init --recursive --depth=1
}

build() {
   cd "\${pkgname}" || exit 1
   cmake -B build \\
      -DCMAKE_BUILD_TYPE=Release \\
      -DCMAKE_INSTALL_PREFIX=/usr
   make -C build
}

package() {
   cd "\${pkgname}" || exit 1
   make -C build DESTDIR=\${pkgdir} install
}
EOF
