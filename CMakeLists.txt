cmake_minimum_required(VERSION 3.25)
project(CANSecKit VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default install location: ~/Documents/Libraries/CANSec
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/Users/furkanozen/Documents/Libraries/CANSec" CACHE PATH "Install path" FORCE)
endif()

option(CANSEC_BUILD_EXAMPLES "Build sender/receiver example apps" ON)

add_library(CANSecKit STATIC
    CANSec.cpp
    CANFDStructure/CANFD.cpp
    CANXLStructure/CANXL.cpp
)
add_library(CANSec::Kit ALIAS CANSecKit)

# Backward-compatible alias name for existing consumers
add_library(CANXL::CANXL ALIAS CANSecKit)

target_include_directories(CANSecKit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/CANFDStructure>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/CANXLStructure>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third-party-library/canbus-common/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/CANSec>
)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(CANSecKit
    PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

set(CANBUS_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party-library/canbus-common")
if(EXISTS "${CANBUS_COMMON_DIR}/libcanbus_common.so")
    add_library(canbus-common SHARED IMPORTED)
    set_target_properties(canbus-common PROPERTIES
        IMPORTED_LOCATION "${CANBUS_COMMON_DIR}/libcanbus_common.so"
        INTERFACE_INCLUDE_DIRECTORIES "${CANBUS_COMMON_DIR}"
    )
    target_link_libraries(CANSecKit PUBLIC canbus-common)
endif()

if(CANSEC_BUILD_EXAMPLES)
    add_executable(canxl_sender tests/CANXL.sender.cpp)
    target_link_libraries(canxl_sender PRIVATE CANSecKit Threads::Threads)

    add_executable(canxl_receiver tests/CANXL_receiver.cpp)
    target_link_libraries(canxl_receiver PRIVATE CANSecKit Threads::Threads)
endif()

install(TARGETS CANSecKit
    EXPORT CANSecKitTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CANSec
)

# Install all public headers (preserve folders to avoid filename collisions)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/CANSec.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CANSec
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/CANFDStructure/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CANSec/CANFDStructure
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/CANXLStructure/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CANSec/CANXLStructure
    FILES_MATCHING PATTERN "*.h"
)

if(EXISTS "${CANBUS_COMMON_DIR}/libcanbus_common.so")
    install(FILES "${CANBUS_COMMON_DIR}/libcanbus_common.so"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CANSecKitConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CANSecKitConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CANSecKitConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CANSecKit
)

install(EXPORT CANSecKitTargets
    FILE CANSecKitTargets.cmake
    NAMESPACE CANSec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CANSecKit
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CANSecKitConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CANSecKitConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CANSecKit
)
