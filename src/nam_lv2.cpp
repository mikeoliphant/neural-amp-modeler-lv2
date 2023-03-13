#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>

// LV2
#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>
#include <lv2/worker/worker.h>

#include "nam_plugin.h"

// LV2 Functions
static LV2_Handle instantiate(
	const LV2_Descriptor*,
	double rate,
	const char*,
	const LV2_Feature* const* features
) {
	LV2_URID_Map* map = nullptr;
	LV2_Log_Logger logger = {};

	for (size_t i = 0; features[i]; ++i) {
		if (std::string(features[i]->URI) == std::string(LV2_URID__map))
			map = static_cast<LV2_URID_Map*>(features[i]->data);
		else if (std::string(features[i]->URI) == std::string(LV2_LOG__log))
			logger.log = static_cast<LV2_Log_Log*>(features[i]->data);
	}

	lv2_log_logger_set_map(&logger, map);

	if (!map) {
		lv2_log_error(&logger, "Missing required feature: `%s`", LV2_URID__map);
		return nullptr;
	}

	try {
		auto nam = std::make_unique<NAM::Plugin>(static_cast<float>(rate));
		nam->map_uris(map);
		return static_cast<LV2_Handle>(nam.release());
	} catch(const std::exception& e) {
		lv2_log_error(&logger, "Failed to instantiate plugin: %s", e.what());
		return nullptr;
	}
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
	auto nam = static_cast<NAM::Plugin*>(instance);
  *(reinterpret_cast<void**>(&nam->ports)+port) = data;
}

static void activate(LV2_Handle) {}

static void run(LV2_Handle instance, uint32_t n_samples) {

	static_cast<NAM::Plugin*>(instance)->process(n_samples);
}

static void deactivate(LV2_Handle) {}

static void cleanup(LV2_Handle instance) {
	delete static_cast<NAM::Plugin*>(instance);
}

static LV2_Worker_Status
work(LV2_Handle                  instance,
	LV2_Worker_Respond_Function respond,
	LV2_Worker_Respond_Handle   handle,
	uint32_t                    size,
	const void* data)
{

	return LV2_WORKER_SUCCESS;
}

static LV2_Worker_Status
work_response(LV2_Handle  instance,
	uint32_t    size,
	const void* data)
{
	return LV2_WORKER_SUCCESS;
}

static const void*
extension_data(const char* uri)
{
	static const LV2_Worker_Interface  worker = { work, work_response, NULL };

	if (!strcmp(uri, LV2_WORKER__interface)) {
		return &worker;
	}

	return NULL;
}

static const LV2_Descriptor descriptor = {
	"http://github.com/mikeoliphant/neural-amp-modeler-lv2",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
	return index == 0 ? &descriptor : nullptr;
}
