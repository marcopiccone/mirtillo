cmake_minimum_required(VERSION 3.16)
project(mirtillo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Se non specificato: Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Qt Core only
find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(mirtillo
  src/main.cpp
)

target_link_libraries(mirtillo PRIVATE Qt6::Core)

# Avvisi utili (senza esagerare su embedded)
target_compile_options(mirtillo PRIVATE -Wall -Wextra -Wpedantic)

# Ottimizzazioni “embedded friendly”
target_compile_options(mirtillo PRIVATE -Os)
target_link_options(mirtillo PRIVATE -Wl,--as-needed)

# RPATH verso ../lib se bundi le Qt
set_target_properties(mirtillo PROPERTIES
  INSTALL_RPATH "\$ORIGIN/../lib"
)

# Install rules → comodo con `cmake --install . --prefix /opt/mirtillo`
include(GNUInstallDirs)
install(TARGETS mirtillo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# (Opzionale) target di deploy via scp
add_custom_target(deploy
  COMMAND ${CMAKE_COMMAND} --install . --prefix /opt/mirtillo
  COMMAND scp ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/mirtillo root@10.11.99.1:/opt/mirtillo/bin/
  COMMENT "Installing locally and deploying to Paper Pro"
)