add_library(neural_amp_modeler MODULE
	nam_lv2.cpp
	nam_plugin.cpp
	nam_plugin.hpp
	dsp.h
	dsp.cpp
	get_dsp.cpp
	util.cpp
	util.h
	wavenet.cpp
	wavenet.h
	json.hpp
)

target_compile_features(neural_amp_modeler PUBLIC cxx_std_17)

set_target_properties(neural_amp_modeler
	PROPERTIES
	CXX_VISIBILITY_PRESET hidden
	INTERPROCEDURAL_OPTIMIZATION TRUE
	PREFIX ""
)

# Compile Options

option(FORCE_DISABLE_DENORMALS "Disable denormal numbers before processing" ON)
target_compile_definitions(neural_amp_modeler
	PRIVATE
	"$<$<CONFIG:RELEASE>:NDEBUG>"
	"$<$<BOOL:${FORCE_DISABLE_DENORMALS}>:FORCE_DISABLE_DENORMALS>"
)

# Architecture
if (
	FORCE_DISABLE_DENORMALS
	AND CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(i386)|(i686)|(AMD64)"
)
	if (MSVC)
		target_compile_options(neural_amp_modeler PRIVATE /arch:SSE2)
	else()
		target_compile_options(neural_amp_modeler PRIVATE -msse3)
	endif()
endif()


# Platform

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(neural_amp_modeler PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if (MSVC)
	target_compile_options(neural_amp_modeler PRIVATE
		"$<$<CONFIG:DEBUG>:/W4>"
		"$<$<CONFIG:RELEASE>:/O2>"
	)
else()
	target_compile_options(neural_amp_modeler PRIVATE
		-Wall -Wextra -Wpedantic -Wshadow -Wstrict-aliasing
		-Wunreachable-code -Wdouble-promotion -Weffc++ -Wconversion
		-Wsign-conversion
		"$<$<CONFIG:DEBUG>:-Og;-ggdb;-Werror>"
		"$<$<CONFIG:RELEASE>:-Ofast>"
	)
endif()
