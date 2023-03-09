#include <algorithm>
#include <cmath>
#include <utility>

// Lv2
#include <lv2/atom/util.h>

#include "nam_plugin.hpp"

namespace NAM {
	Plugin::Plugin(float rate)
	{
		namModel = get_dsp("C:\\Users\\oliph\\AppData\\Roaming\\GuitarSim\\NAM\\JCM2000Crunch.nam");
	}

	void Plugin::map_uris(LV2_URID_Map* map) noexcept {
		lv2_atom_forge_init(&atom_forge, map);
		uris.atom_Object = map->map(map->handle, LV2_ATOM__Object);
		uris.atom_Float = map->map(map->handle, LV2_ATOM__Float);
	}

	void Plugin::process(uint32_t n_samples) noexcept {
		namModel->process(ports.audio_in, ports.audio_out, n_samples, 1.0, 1.0, mNAMParams);
		namModel->finalize_(n_samples);
	}
}
