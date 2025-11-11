# local-notes-manager-40726-40735

Notes App Native (Qt6)

- Build:
  - mkdir -p build && cd build
  - cmake -S ../notes_app_native -B . -DCMAKE_BUILD_TYPE=Release
  - cmake --build .
- Run (manual only, never auto-run in CI):
  - From build directory, execute the built binary:
    - ./MainApp
- Tests (headless):
  - ctest --output-on-failure
  - or run NoteUtilsTest directly: ./NoteUtilsTest

CI/headless policy:
- No custom build steps are allowed to run the GUI automatically.
- NoteUtilsTest links only Qt6::Core and Qt6::Test; it must not instantiate QApplication.
- Any convenience 'run' targets must not be added to the repository. If needed locally, configure with:
  - cmake -DENABLE_LOCAL_RUN_TARGETS=ON ...
  and create local, uncommitted helper targets in your user presets.