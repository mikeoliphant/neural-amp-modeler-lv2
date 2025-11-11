#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string_view>

// LV2
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/atom/atom.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>
#include <lv2/urid/urid.h>
#include <lv2/atom/forge.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/options/options.h>
#include <lv2/patch/patch.h>
#include <lv2/worker/worker.h>
#include <lv2/state/state.h>
#include <lv2/units/units.h>

#include <NeuralAudio/NeuralModel.h>

#define PlUGIN_URI "http://github.com/mikeoliphant/neural-amp-modeler-lv2"
#define MODEL_URI PlUGIN_URI "#model"

namespace NAM {
	static constexpr unsigned int MAX_FILE_NAME = 1024;

	enum LV2WorkType {
		kWorkTypeLoad,
		kWorkTypeSwitch,
		kWorkTypeFree
	};

	struct LV2LoadModelMsg {
		LV2WorkType type;
		char path[MAX_FILE_NAME];
	};

	struct LV2SwitchModelMsg {
		LV2WorkType type;
		char path[MAX_FILE_NAME];
		NeuralAudio::NeuralModel* model;
	};

	struct LV2FreeModelMsg {
		LV2WorkType type;
		NeuralAudio::NeuralModel* model;
	};

	class Plugin {
	public:
		struct Ports {
			const LV2_Atom_Sequence* control;
			LV2_Atom_Sequence* notify;
			const float* audio_in;
			float* audio_out;
			float* input_level;
			float* output_level;
		};

		Ports ports = {};

		double sampleRate;

		LV2_URID_Map* map = nullptr;
		LV2_Log_Logger logger = {};
		LV2_Worker_Schedule* schedule = nullptr;

		NeuralAudio::NeuralModel* currentModel = nullptr;
		std::string currentModelPath;
		float prevDCInput = 0;
		float prevDCOutput = 0;

		Plugin();
		~Plugin();

		bool initialize(double rate, const LV2_Feature* const* features) noexcept;
		void set_max_buffer_size(int size) noexcept;
		void activate() noexcept;
		void process(uint32_t n_samples) noexcept;

		void write_current_path();

		static uint32_t options_get(LV2_Handle instance, LV2_Options_Option* options);
		static uint32_t options_set(LV2_Handle instance, const LV2_Options_Option* options);

		static LV2_Worker_Status work(LV2_Handle instance, LV2_Worker_Respond_Function respond, LV2_Worker_Respond_Handle handle,
			uint32_t size, const void* data);
		static LV2_Worker_Status work_response(LV2_Handle instance, uint32_t size, const void* data);

		static LV2_State_Status save(LV2_Handle instance, LV2_State_Store_Function store, LV2_State_Handle handle, uint32_t flags, 
			const LV2_Feature* const* features);
		static LV2_State_Status restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle, uint32_t flags,
			const LV2_Feature* const* features);

	private:
		struct URIs {
			LV2_URID atom_Object;
			LV2_URID atom_Float;
			LV2_URID atom_Int;
			LV2_URID atom_Path;
			LV2_URID atom_URID;
			LV2_URID bufSize_maxBlockLength;
			LV2_URID patch_Set;
			LV2_URID patch_Get;
			LV2_URID patch_property;
			LV2_URID patch_value;
			LV2_URID units_frame;
			LV2_URID model_Path;
		};

		URIs uris = {};

		LV2_Atom_Forge atom_forge = {};
		LV2_Atom_Forge_Frame sequence_frame;

		float inputLevel = 0;
		float outputLevel = 0;
		int32_t maxBufferSize = 512;
		float bypassThresholdLinear = 0;
		uint32_t silentSamples = 0;
		bool smartBypassed = true;
	};
}
