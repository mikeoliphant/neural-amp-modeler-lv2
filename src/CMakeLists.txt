add_library(neural_amp_modeler MODULE
	nam_lv2.cpp
	nam_plugin.cpp
	nam_plugin.h
	../NeuralAmpModelerCore/NAM/activations.h
	../NeuralAmpModelerCore/NAM/version.h
	../NeuralAmpModelerCore/NAM/lstm.h
	../NeuralAmpModelerCore/NAM/lstm.cpp
	../NeuralAmpModelerCore/NAM/dsp.h
	../NeuralAmpModelerCore/NAM/dsp.cpp
	../NeuralAmpModelerCore/NAM/get_dsp.cpp
	../NeuralAmpModelerCore/NAM/util.cpp
	../NeuralAmpModelerCore/NAM/util.h
	../NeuralAmpModelerCore/NAM/wavenet.cpp
	../NeuralAmpModelerCore/NAM/wavenet.h
)

target_compile_features(neural_amp_modeler PUBLIC cxx_std_17)

set_target_properties(neural_amp_modeler
	PROPERTIES
	CXX_VISIBILITY_PRESET hidden
	INTERPROCEDURAL_OPTIMIZATION TRUE
	PREFIX ""
)


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
