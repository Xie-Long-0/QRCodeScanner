#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ZXing::ZXing" for configuration "Release"
set_property(TARGET ZXing::ZXing APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(ZXing::ZXing PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/ZXing.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/ZXing.dll"
  )

list(APPEND _cmake_import_check_targets ZXing::ZXing )
list(APPEND _cmake_import_check_files_for_ZXing::ZXing "${_IMPORT_PREFIX}/lib/ZXing.lib" "${_IMPORT_PREFIX}/bin/ZXing.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
