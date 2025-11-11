cmake_minimum_required(VERSION 3.16)
project(MainApp VERSION 1.0.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt's automatic processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# On some environments with newer CMake/Qt6, set policies explicitly
if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW) # AUTOMOC for generated files
endif()

# Include headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# We intentionally separate Qt component discovery for GUI app vs. headless tests to
# avoid pulling in Qt Widgets for the test target, which would require XKB/GUI deps.
# Find for GUI application
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
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
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
find_package(Qt6 REQUIRED COMPONENTS Core Test)

# Unit test for NoteUtils (console) - GUI-free
add_executable(NoteUtilsTest
    tests/note_utils_test.cpp
    tests/note_utils_impl.cpp
    include/note_utils.h
)
target_include_directories(NoteUtilsTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(NoteUtilsTest PRIVATE Qt6::Core Qt6::Test)

# Ensure tests run in headless environments; they do not create QApplication/QWidget.
add_test(NAME NoteUtilsTest COMMAND NoteUtilsTest)
