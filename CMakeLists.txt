cmake_minimum_required(VERSION 3.16)
project(mirtillo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(mirtillo
  src/main.cpp
  src/logging.cpp
  src/json_utils.cpp
  src/scanner.cpp
  src/export.cpp
)

target_link_libraries(mirtillo PRIVATE Qt6::Core)

# Provide project version to the source code
target_compile_definitions(mirtillo PRIVATE
    MIRTILLO_VERSION="${PROJECT_VERSION}"
    MIRTILLO_ABOUT_PATH="${CMAKE_SOURCE_DIR}/ABOUT.txt"
)

target_compile_options(mirtillo PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(mirtillo PRIVATE -Os)
target_link_options(mirtillo PRIVATE -Wl,--as-needed)

set_target_properties(mirtillo PROPERTIES
  INSTALL_RPATH "\$ORIGIN/../lib"
)

include(GNUInstallDirs)
install(TARGETS mirtillo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})