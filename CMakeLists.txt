cmake_minimum_required(VERSION 3.25)
project(CANSecKit VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default install location:
# - inside the devcontainer, use the host-mounted libraries path
# - locally, use ~/Documents/Libraries/CANSec
if(EXISTS "/host_libraries")
    set(_cansec_default_install_prefix "/host_libraries/CANSec")
elseif(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
    set(_cansec_default_install_prefix "$ENV{HOME}/Documents/Libraries/CANSec")
else()
    set(_cansec_default_install_prefix "${CMAKE_CURRENT_SOURCE_DIR}/_install")
endif()

if(DEFINED ENV{CANSEC_INSTALL_PREFIX} AND NOT "$ENV{CANSEC_INSTALL_PREFIX}" STREQUAL "")
    set(_cansec_install_prefix "$ENV{CANSEC_INSTALL_PREFIX}")
elseif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR
       CMAKE_INSTALL_PREFIX STREQUAL "/usr/local" OR
       CMAKE_INSTALL_PREFIX STREQUAL "")
    set(_cansec_install_prefix "${_cansec_default_install_prefix}")
endif()

if(DEFINED _cansec_install_prefix)
    set(CMAKE_INSTALL_PREFIX "${_cansec_install_prefix}" CACHE PATH "Install path" FORCE)
    unset(_cansec_install_prefix)
endif()
unset(_cansec_default_install_prefix)

option(CANSEC_BUILD_EXAMPLES "Build sender/receiver example apps" ON)

set(_cansec_sources
    CANSec.cpp
)

set(_cansec_can_sources
    CANFDStructure/CANFD.cpp
    CANXLStructure/CANXL.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND _cansec_sources ${_cansec_can_sources})
else()
    message(STATUS "Building portable CANSecKit without SocketCAN sources on ${CMAKE_SYSTEM_NAME}")
endif()

add_library(CANSecKit STATIC ${_cansec_sources})
unset(_cansec_can_sources)
unset(_cansec_sources)
add_library(CANSec::Kit ALIAS CANSecKit)

# Backward-compatible alias name for existing consumers
add_library(CANXL::CANXL ALIAS CANSecKit)

target_include_directories(CANSecKit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/CANFDStructure>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/CANXLStructure>
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

if(CANSEC_BUILD_EXAMPLES AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
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
