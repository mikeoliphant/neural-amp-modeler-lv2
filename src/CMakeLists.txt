set(SOURCES nam_lv2.cpp
	nam_plugin.cpp
	nam_plugin.h)

set(NAM_SOURCES	../deps/NeuralAmpModelerCore/NAM/activations.h
	../deps/NeuralAmpModelerCore/NAM/activations.cpp
	../deps/NeuralAmpModelerCore/NAM/version.h
	../deps/NeuralAmpModelerCore/NAM/lstm.h
	../deps/NeuralAmpModelerCore/NAM/lstm.cpp
	../deps/NeuralAmpModelerCore/NAM/dsp.h
	../deps/NeuralAmpModelerCore/NAM/dsp.cpp
	../deps/NeuralAmpModelerCore/NAM/get_dsp.cpp
	../deps/NeuralAmpModelerCore/NAM/util.cpp
	../deps/NeuralAmpModelerCore/NAM/util.h
	../deps/NeuralAmpModelerCore/NAM/wavenet.cpp
	../deps/NeuralAmpModelerCore/NAM/wavenet.h
	../deps/NeuralAmpModelerCore/NAM/convnet.cpp
	../deps/NeuralAmpModelerCore/NAM/convnet.h)

add_library(neural_amp_modeler MODULE ${SOURCES} ${NAM_SOURCES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group(NAM ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NAM_SOURCES})

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
		-Wall -Wextra -Wpedantic -Wstrict-aliasing -Wunreachable-code -Weffc++ -Wno-unused-parameter
		"$<$<CONFIG:DEBUG>:-Og;-ggdb;-Werror>"
		"$<$<CONFIG:RELEASE>:-Ofast>"
	)
endif()
