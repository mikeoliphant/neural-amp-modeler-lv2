#include <algorithm>
#include <cmath>
#include <utility>

// Lv2
#include <lv2/atom/util.h>
#include <lv2/patch/patch.h>

#include "nam_plugin.hpp"

namespace NAM {
	Plugin::Plugin(float rate)
	{
		namModel = get_dsp("C:\\Users\\oliph\\AppData\\Roaming\\GuitarSim\\NAM\\AC15Brkup.nam");
	}

	void Plugin::map_uris(LV2_URID_Map* map) noexcept {
		lv2_atom_forge_init(&atom_forge, map);

		uris.atom_Object = map->map(map->handle, LV2_ATOM__Object);
		uris.atom_Float = map->map(map->handle, LV2_ATOM__Float);
		uris.atom_Int = map->map(map->handle, LV2_ATOM__Int);
		uris.atom_Path = map->map(map->handle, LV2_ATOM__Path);
		uris.atom_URID = map->map(map->handle, LV2_ATOM__URID);
		uris.patch_Set = map->map(map->handle, LV2_PATCH__Set);
		uris.patch_property = map->map(map->handle, LV2_PATCH__property);
		uris.patch_value = map->map(map->handle, LV2_PATCH__value);

		uris.model_Path = map->map(map->handle, MODEL_URI);
	}

	void Plugin::process(uint32_t n_samples) noexcept {
		namModel->process(ports.audio_in, ports.audio_out, n_samples, 1.0, 1.0, mNAMParams);
		namModel->finalize_(n_samples);
	}
}
