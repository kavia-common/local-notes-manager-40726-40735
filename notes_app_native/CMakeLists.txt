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

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)

# Include headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Sources and headers
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

# Tests
enable_testing()

# Unit test for NoteUtils (console)
add_executable(NoteUtilsTest tests/note_utils_test.cpp include/mainApp.h)
target_link_libraries(NoteUtilsTest PRIVATE Qt6::Core Qt6::Test)
add_test(NAME NoteUtilsTest COMMAND NoteUtilsTest)
