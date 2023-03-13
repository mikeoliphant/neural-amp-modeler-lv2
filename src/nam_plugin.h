#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string_view>

// LV2
#include <lv2/atom/atom.h>
#include <lv2/urid/urid.h>
#include <lv2/atom/forge.h>

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

		std::unique_ptr<::DSP> namModel;

		std::unordered_map<std::string, double> mNAMParams =
		{
			{"Input", 0.0},
			{"Output", 0.0}
		};

		/*
			Member Functions
		*/

		Plugin(float rate);
		~Plugin() = default;

		void map_uris(LV2_URID_Map* map) noexcept;

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
