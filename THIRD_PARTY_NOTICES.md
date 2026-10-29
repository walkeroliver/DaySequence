# Third-party components

DaySequence source is licensed under GPL-3.0-only. The Windows binary package dynamically links the unmodified Qt 6.8.3 and MinGW runtime libraries.

## Qt 6.8.3

Modules: Qt Core, Gui, Widgets, Network and Svg, plus the corresponding Windows, image-format, icon, network and TLS plugins.
Copyright The Qt Company Ltd. and other Qt contributors. Qt is available under LGPL-3.0 / GPL-3.0 and commercial terms; the open-source libraries in this package retain their upstream licenses.

- GPL version 3: `LICENSE`.
- LGPL version 3: `licenses/LGPL-3.0-only.txt`.
- Bundled component copyright and license texts: `licenses/Qt-THIRD-PARTY-NOTICES.txt`.
- Upstream bill of materials: `licenses/qtbase-6.8.3.spdx.json` and `licenses/qtsvg-6.8.3.spdx.json`.
- Matching source: https://download.qt.io/archive/qt/6.8/6.8.3/submodules/qtbase-everywhere-src-6.8.3.tar.xz
- Matching source: https://download.qt.io/archive/qt/6.8/6.8.3/submodules/qtsvg-everywhere-src-6.8.3.tar.xz
- Qt licenses: https://doc.qt.io/qt-6.8/licensing.html

The DLLs remain separately replaceable. No application restriction is imposed on replacing these libraries or reverse engineering for debugging modifications to them.

## MinGW runtime

Windows MinGW builds include libgcc_s_seh-1.dll, libstdc++-6.dll and libwinpthread-1.dll.
Their upstream notices are included in `licenses/mingw/`. GCC runtime libraries use the GCC Runtime Library Exception in addition to their GPL license. MinGW-w64 and winpthreads retain their respective upstream license terms.

Sources: https://gcc.gnu.org/releases.html and https://www.mingw-w64.org/.

## Weather data

Weather is queried from Open-Meteo only when enabled by the user. No weather dataset is bundled in the application.
Source and attribution: https://open-meteo.com/ and https://open-meteo.com/en/terms.
