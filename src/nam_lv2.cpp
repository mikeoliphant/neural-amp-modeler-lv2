#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>

// LV2
#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>
#include <lv2/worker/worker.h>

#include "nam_plugin.h"

// LV2 Functions
static LV2_Handle instantiate(const LV2_Descriptor*, double rate, const char*, const LV2_Feature* const* features
) {
	try
	{
		auto nam = std::make_unique<NAM::Plugin>(static_cast<float>(rate));

		if (nam->initialize(rate, features))
		{
			return static_cast<LV2_Handle>(nam.release());
		}

		return nullptr;
	}
	catch(const std::exception& e)
	{
		return nullptr;
	}
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	auto nam = static_cast<NAM::Plugin*>(instance);
  *(reinterpret_cast<void**>(&nam->ports)+port) = data;
}

static void activate(LV2_Handle) {}

static void run(LV2_Handle instance, uint32_t n_samples)
{
	static_cast<NAM::Plugin*>(instance)->process(n_samples);
}

static void deactivate(LV2_Handle) {}

static void cleanup(LV2_Handle instance)
{
	delete static_cast<NAM::Plugin*>(instance);
}

static const void* extension_data(const char* uri)
{
	static const LV2_Worker_Interface  worker = { NAM::Plugin::work, NAM::Plugin::work_response, NULL };

	if (!strcmp(uri, LV2_WORKER__interface))
		return &worker;

	return NULL;
}

static const LV2_Descriptor descriptor =
{
	"http://github.com/mikeoliphant/neural-amp-modeler-lv2",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
	return index == 0 ? &descriptor : nullptr;
}
