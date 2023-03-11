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
		if (ports.control) {
			LV2_ATOM_SEQUENCE_FOREACH(ports.control, event) {
				if (event->body.type == uris.atom_Object) {
					const auto obj = reinterpret_cast<LV2_Atom_Object*>(&event->body);

					if (obj->body.otype == uris.patch_Set)
					{
						const LV2_Atom* property = NULL;
						const LV2_Atom* file_path = NULL;

						lv2_atom_object_get(obj, uris.patch_property, &property, 0);

						if (property && (property->type == uris.atom_URID))
						{
							if (((const LV2_Atom_URID*)property)->body == uris.model_Path)
							{
								lv2_atom_object_get(obj, uris.patch_value, &file_path, 0);

								if (file_path && (file_path->size > 0))
								{
									namModel = get_dsp((const char*)LV2_ATOM_BODY_CONST(file_path));
								}
							}
						}
					}
				}
			}
		}

		if (namModel == nullptr)
		{
			for (int i = 0; i < n_samples; i++)
			{
				ports.audio_out[i] = ports.audio_in[i];
			}
		}
		else
		{
			namModel->process(ports.audio_in, ports.audio_out, n_samples, 1.0, 1.0, mNAMParams);
			namModel->finalize_(n_samples);
		}
	}
}
