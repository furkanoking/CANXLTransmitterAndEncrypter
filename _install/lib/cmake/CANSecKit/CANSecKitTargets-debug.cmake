#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "CANSec::CANSecKit" for configuration "Debug"
set_property(TARGET CANSec::CANSecKit APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(CANSec::CANSecKit PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libCANSecKit.a"
  )

list(APPEND _cmake_import_check_targets CANSec::CANSecKit )
list(APPEND _cmake_import_check_files_for_CANSec::CANSecKit "${_IMPORT_PREFIX}/lib/libCANSecKit.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
