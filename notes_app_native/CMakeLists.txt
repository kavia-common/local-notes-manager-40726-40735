cmake_minimum_required(VERSION 3.16)
project(MainApp VERSION 1.0.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Global AUTOGEN off by default to avoid accidental GUI autogen in tests
# We'll enable AUTOGEN only for the GUI target to keep tests headless-safe.
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# On some environments with newer CMake/Qt6, set policies explicitly
if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW) # AUTOMOC for generated files
endif()

# -----------------------------
# Main GUI application
# -----------------------------
# Find for GUI application ONLY here. Do not expose Widgets to tests.
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Sources and headers for GUI app
set(SOURCES
    src/mainApp.cpp
)
set(HEADERS
    include/mainApp.h
)

# Main app (GUI)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Enable AUTOGEN only for GUI target
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Widgets
)

# Convenience run target for CI/dev
add_custom_target(run
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ${PROJECT_NAME}"
)

# -----------------------------
# Tests - strictly headless
# -----------------------------
enable_testing()

# Only find Core and Test for NoteUtilsTest; DO NOT require Widgets here.
# This prevents any transitive pull of GUI deps into test target.
find_package(Qt6 REQUIRED COMPONENTS Core Test)

# Unit test for NoteUtils (console) - GUI-free
# Note: Do not broadly add include/ directory; explicitly expose only note_utils.h via a private interface dir.
add_executable(NoteUtilsTest
    tests/note_utils_test.cpp
    tests/note_utils_impl.cpp
)

# Disable AUTOGEN explicitly for tests (guard against CMake defaults or future policy changes)
set_target_properties(NoteUtilsTest PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

# Restrict include paths so we do NOT expose include/ containing mainApp.h to the test target.
# Provide only the include path for note_utils.h (same include folder), but avoid leaking other headers by including directory
# while the test sources include only "note_utils.h".
# We keep it PRIVATE to ensure it doesn't propagate.
target_include_directories(NoteUtilsTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link strictly headless Qt components for tests
target_link_libraries(NoteUtilsTest
    PRIVATE
        Qt6::Core
        Qt6::Test
)

# Prevent accidental linking against Widgets through whole-archive or transitive deps.
# Many linkers ignore this if there's no symbol reference, but this documents intent and avoids accidental pulls.
# No Widgets target is specified here by design.

# Ensure tests run in headless environments; they do not create QApplication/QWidget.
add_test(NAME NoteUtilsTest COMMAND NoteUtilsTest)
