# tools/checks

Standalone harnesses. **Not wired into the main build** — the project's
CMakeLists.txt is untouched, so these cost you nothing until you want them.

    cmake -S tools/checks -B tools/checks/build \
          -DCMAKE_PREFIX_PATH=$HOME/Qt/6.11.1/macos -DCMAKE_BUILD_TYPE=Release
    cmake --build tools/checks/build -j8
    QT_QPA_PLATFORM=offscreen ./tools/checks/build/uim2smoke
    QT_QPA_PLATFORM=offscreen ./tools/checks/build/uim2grid

`smoke.cpp` — three assertions that are hard to reach by clicking:
  * a populated SceneDocument tears down without a double-free
  * UiBinReader rejects hostile lengths instead of reading out of bounds
  * a bake still round-trips through UiBinReader::Validate

`gridbench.cpp` — times the old per-ellipse dot grid against the current tiled
blit at several zoom levels, and dumps PNGs so you can eyeball the tiling phase.

These are the seed for the plan's stage 8 (a UIMaker2Checks target sharing the
source list). Note: do NOT switch to qt_add_library(... STATIC) for that — the
REGISTER_COMPONENT file-scope initialisers get discarded by the linker and
Component::Registry() comes back silently empty.
