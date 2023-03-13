#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string_view>

// LV2
#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>
#include <lv2/urid/urid.h>
#include <lv2/atom/forge.h>
#include <lv2/patch/patch.h>
#include <lv2/worker/worker.h>

#include "dsp.h"

#define PlUGIN_URI "http://github.com/mikeoliphant/neural-amp-modeler-lv2"
#define MODEL_URI PlUGIN_URI "#model"

namespace NAM {

	class Plugin {
	public:
		struct Ports {
			const LV2_Atom_Sequence* control;
			LV2_Atom_Sequence* notify;
			const float* audio_in;
			float* audio_out;
		};

		Ports ports = {};

		LV2_URID_Map* map;
		LV2_Log_Logger logger;
		LV2_Worker_Schedule* schedule;

		std::unique_ptr<::DSP> namModel;

		std::unordered_map<std::string, double> mNAMParams =
		{
			{"Input", 0.0},
			{"Output", 0.0}
		};


		Plugin(float rate);
		~Plugin() = default;

		bool initialize(double rate, const LV2_Feature* const* features) noexcept;
		void process(uint32_t n_samples) noexcept;

	private:
		struct URIs {
			LV2_URID atom_Object;
			LV2_URID atom_Float;
			LV2_URID atom_Int;
			LV2_URID atom_Path;
			LV2_URID atom_URID;
			LV2_URID patch_Set;
			LV2_URID patch_property;
			LV2_URID patch_value;
			LV2_URID model_Path;
		};

		URIs uris = {};
		LV2_Atom_Forge atom_forge = {};

		float m_rate;
	};
}
