cmake_minimum_required(VERSION 3.10)

project(NeuralAmpModelerLv2 VERSION 0.1.4)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	include_directories(SYSTEM /usr/local/include)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	link_libraries(stdc++fs)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
else()
	message(FATAL_ERROR "Unrecognized Platform!")
endif()

set(NAM_LV2_ID http://github.com/mikeoliphant/neural-amp-modeler-lv2)

include_directories(SYSTEM deps/eigen)
include_directories(SYSTEM deps/lv2/include)
include_directories(SYSTEM deps/NeuralAmpModelerCore)
include_directories(SYSTEM deps/json)
include_directories(SYSTEM deps/denormal)

add_definitions(-DNAM_SAMPLE_FLOAT)
add_definitions(-DDSP_SAMPLE_FLOAT)

add_subdirectory(src)


# create neural_amp_modeler.lv2
add_custom_target(copy_binaries ALL
	${CMAKE_COMMAND} -E copy "$<TARGET_FILE:neural_amp_modeler>" neural_amp_modeler.lv2/
	DEPENDS neural_amp_modeler
)

configure_file(resources/manifest.ttl.in neural_amp_modeler.lv2/manifest.ttl)
configure_file(resources/neural_amp_modeler.ttl.in neural_amp_modeler.lv2/neural_amp_modeler.ttl)

install (DIRECTORY ${CMAKE_BINARY_DIR}/neural_amp_modeler.lv2
       DESTINATION lib/lv2
)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "oliphant@nostatic.org>")
set(CPACK_PACKAGE_VENDOR "Mike Oliphant")
set(CPACK_PACKAGE_DESCRIPTION "Neural Amp Modeler LV2 plugin implementation")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Neural Amp Modeler LV2 plugin")
set(CPACK_DEBIAN_PACKAGE_SECTION sound)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set (CPACK_STRIP_FILES ON)

include(CPack)
