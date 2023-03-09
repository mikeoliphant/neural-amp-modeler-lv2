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

namespace NAM {

	class Plugin {
	public:
		static constexpr std::string_view URI = "http://github.com/mikeoliphant/neural-amp-modeler-lv2";

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
		};

		URIs uris = {};
		LV2_Atom_Forge atom_forge = {};

		float m_rate;
	};
}
