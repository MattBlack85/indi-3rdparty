cmake_minimum_required(VERSION 3.18)
PROJECT(indi_pentax CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules/")

include(GNUInstallDirs)

set (INDI_PENTAX_VERSION_MAJOR 1)
set (INDI_PENTAX_VERSION_MINOR 2)

find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(LibRaw REQUIRED)
find_package(JPEG REQUIRED)
find_package(PENTAX REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_pentax.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_pentax.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})
include_directories( ${LibRaw_INCLUDE_DIR})
include_directories( ${PENTAX_INCLUDE_DIR})
include_directories( ${PKTRIGGERCORD_INCLUDE_DIR})

include(CMakeCommon)
############# PENTAX CCD ###############
set(indibase_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/indi_pentax.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pktriggercord_ccd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gphoto_readimage.cpp
)
set(indiricoh_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/pentax_ccd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pentax_event_handler.cpp
)
#ricoh sdk does not have armv8 binary, so don't include it for aarch64 platforms
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^aarch64")
    set(indipentax_SRCS ${indibase_SRCS})
else()
    set(indipentax_SRCS ${indibase_SRCS} ${indiricoh_SRCS})
endif()

add_executable(indi_pentax ${indipentax_SRCS})

target_link_libraries(indi_pentax pthread ${PENTAX_LIBRARIES} ${INDI_LIBRARIES} ${JPEG_LIBRARIES} ${LibRaw_LIBRARIES} ${CFITSIO_LIBRARIES} ${ZLIB_LIBRARY})

install(TARGETS indi_pentax RUNTIME DESTINATION bin)

# Some systems (like ArchLinux) cannot take advantage of this feature and setcap should be set post-install
if(NOT DEFINED SKIP_SETCAP)
  find_program(SUDO sudo)
  if(NOT SUDO)
    message(WARNING "sudo must be available on the system in order to set permissions for the driver")
  else()
    message("-- sudo command: ${SUDO}")
  endif()

  find_program(SETCAP setcap)
  if(NOT SETCAP)
    message(WARNING "setcap not found, driver may not work because of lack of permissions")
  else()
    message("-- setcap command: ${SETCAP}")
    install(CODE "execute_process(COMMAND ${SUDO} -S ${SETCAP} cap_sys_rawio+ep ${CMAKE_INSTALL_PREFIX}/bin/indi_pentax)")
  endif()
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_pentax.xml DESTINATION ${INDI_DATA_DIR})
