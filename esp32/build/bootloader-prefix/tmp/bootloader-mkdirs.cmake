# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/grzes/esp/v5.2.1/esp-idf/components/bootloader/subproject"
  "C:/Users/grzes/MatiEsp/build/bootloader"
  "C:/Users/grzes/MatiEsp/build/bootloader-prefix"
  "C:/Users/grzes/MatiEsp/build/bootloader-prefix/tmp"
  "C:/Users/grzes/MatiEsp/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/grzes/MatiEsp/build/bootloader-prefix/src"
  "C:/Users/grzes/MatiEsp/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/grzes/MatiEsp/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/grzes/MatiEsp/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()