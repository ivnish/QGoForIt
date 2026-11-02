#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${ROOT}/build-appimage"
TOOLS_DIR="${BUILD_ROOT}/tools"
APPDIR="${BUILD_ROOT}/appdir"
VERSION="$(sed -n 's/^project(QGoForIt VERSION \([0-9.]*\).*/\1/p' "${ROOT}/CMakeLists.txt")"
OUTPUT="${ROOT}/QGoForIt-${VERSION}-x86_64.AppImage"

mkdir -p "${TOOLS_DIR}"

if [ -x /usr/lib/qt6/bin/qmake ] && /usr/lib/qt6/bin/qmake -query QT_VERSION | grep -q '^6'; then
  export PATH="/usr/lib/qt6/bin:${PATH}"
elif command -v qmake6 >/dev/null 2>&1; then
  QT6_BIN_DIR="$(dirname "$(readlink -f "$(command -v qmake6)")")"
  if [ -x "${QT6_BIN_DIR}/qmake" ]; then
    export PATH="${QT6_BIN_DIR}:${PATH}"
  fi
fi

download() {
  local url="$1" dest="$2"
  if [ ! -f "${dest}" ]; then
    curl -L --fail -o "${dest}" "${url}"
  fi
  chmod +x "${dest}"
}

download "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
  "${TOOLS_DIR}/linuxdeploy"
download "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
  "${TOOLS_DIR}/linuxdeploy-plugin-qt"

export PATH="${TOOLS_DIR}:${PATH}"
export APPIMAGE_EXTRACT_AND_RUN=1

cmake -B "${BUILD_ROOT}/build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "${BUILD_ROOT}/build" -j"$(nproc)"

rm -rf "${APPDIR}"
cmake --install "${BUILD_ROOT}/build" --prefix "${APPDIR}/usr"

sed -i "s|^Icon=.*|Icon=QGoForIt|" "${APPDIR}/usr/share/applications/QGoForIt.desktop"

cd "${BUILD_ROOT}"
linuxdeploy \
  --appdir "${APPDIR}" \
  -d "${APPDIR}/usr/share/applications/QGoForIt.desktop" \
  -i "${ROOT}/icons/QGoForIt.png" \
  --plugin qt \
  --output appimage

rm -f "${OUTPUT}"
mv "${BUILD_ROOT}/QGoForIt-x86_64.AppImage" "${OUTPUT}"

echo "Готово: ${OUTPUT}"
