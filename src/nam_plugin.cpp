#include <algorithm>
#include <cmath>
#include <utility>

#include "nam_plugin.h"
#include "activations.h"
#include <cassert>

#define SMOOTH_EPSILON .0001f

namespace NAM {
	Plugin::Plugin()
	{
		// prevent allocations on the audio thread
		currentModelPath.reserve(MAX_FILE_NAME+1);
	}

	Plugin::~Plugin()
	{
		delete currentModel;
	}

	bool Plugin::initialize(double rate, const LV2_Feature* const* features) noexcept
	{
		// for fetching initial options, can be null
		LV2_Options_Option* options = nullptr;

		for (size_t i = 0; features[i]; ++i) {
			if (std::string(features[i]->URI) == std::string(LV2_URID__map))
				map = static_cast<LV2_URID_Map*>(features[i]->data);
			else if (std::string(features[i]->URI) == std::string(LV2_WORKER__schedule))
				schedule = static_cast<LV2_Worker_Schedule*>(features[i]->data);
			else if (std::string(features[i]->URI) == std::string(LV2_LOG__log))
				logger.log = static_cast<LV2_Log_Log*>(features[i]->data);
			else if (std::string(features[i]->URI) == std::string(LV2_OPTIONS__options))
				options = static_cast<LV2_Options_Option*>(features[i]->data);
		}
	
		lv2_log_logger_set_map(&logger, map);

		if (!map)
		{
			lv2_log_error(&logger, "Missing required feature: `%s`", LV2_URID__map);

			return false;
		}

		if (!schedule)
		{
			lv2_log_error(&logger, "Missing required feature: `%s`", LV2_WORKER__schedule);

			return false;
		}

		lv2_atom_forge_init(&atom_forge, map);

		uris.atom_Object = map->map(map->handle, LV2_ATOM__Object);
		uris.atom_Float = map->map(map->handle, LV2_ATOM__Float);
		uris.atom_Int = map->map(map->handle, LV2_ATOM__Int);
		uris.atom_Path = map->map(map->handle, LV2_ATOM__Path);
		uris.atom_URID = map->map(map->handle, LV2_ATOM__URID);
		uris.bufSize_maxBlockLength = map->map(map->handle, LV2_BUF_SIZE__maxBlockLength);
		uris.patch_Set = map->map(map->handle, LV2_PATCH__Set);
		uris.patch_Get = map->map(map->handle, LV2_PATCH__Get);
		uris.patch_property = map->map(map->handle, LV2_PATCH__property);
		uris.patch_value = map->map(map->handle, LV2_PATCH__value);
		uris.units_frame = map->map(map->handle, LV2_UNITS__frame);

		uris.model_Path = map->map(map->handle, MODEL_URI);

		if (options != nullptr)
			options_set(this, options);

		return true;
	}

	::DSP* Plugin::load_model(const char*filename)
	{
		// runs on non-RT, can block or use [de]allocations
			
		::DSP* model;
		const size_t pathlen = strlen(filename);
		if (pathlen == 0 || pathlen >= MAX_FILE_NAME)
		{
			// avoid logging an error on an empty path.
			// but do clear the model.
			model = nullptr;
		}
		else
		{
			lv2_log_trace(&logger, "Staging model change: `%s`\n", filename);

			model = get_dsp(filename).release();
			if (model != nullptr)
			{
				// Enable model loudness normalization
				model->SetNormalize(true);

				// Pre-run model to ensure all needed buffers are allocated in advance
				if (const int32_t numSamples = maxBufferSize)
				{
					float* buffer = new float[numSamples];

					std::unordered_map<std::string, double> params = {};
					model->process(&buffer, &buffer, 1, numSamples, 1.0, 1.0, params);
					model->finalize_(numSamples);

					delete[] buffer;
				}
			}
		}
		return model;
	}
	// runs on non-RT, can block or use [de]allocations
	LV2_Worker_Status Plugin::work(LV2_Handle instance, LV2_Worker_Respond_Function respond, LV2_Worker_Respond_Handle handle,
		uint32_t size, const void* data)
	{
		switch (*(const LV2WorkType*)data)
		{
			case kWorkTypeLoad:
			{
				auto msg = static_cast<const LV2LoadModelMsg*>(data);
				auto nam = static_cast<NAM::Plugin*>(instance);

				try
				{
				
					// load model from path
					::DSP* model = nam->load_model(msg->path);

					LV2SwitchModelMsg response = { kWorkTypeSwitch, {}, model };
					memcpy(response.path, msg->path, strlen(msg->path));
					respond(handle, sizeof(response), &response);

					return LV2_WORKER_SUCCESS;
				}
				catch (std::exception& e)
				{
					lv2_log_error(&nam->logger, "Unable to load model from: '%s'\n", msg->path);
				}

				break;
			}

			case kWorkTypeFree:
			{
				auto msg = static_cast<const LV2FreeModelMsg*>(data);
				delete msg->model;
				return LV2_WORKER_SUCCESS;
			}

			case kWorkTypeSwitch:
				// should not happen!
				break;
		}

		return LV2_WORKER_ERR_UNKNOWN;
	}

	// runs on RT, right after process(), must not block or [de]allocate memory
	LV2_Worker_Status Plugin::work_response(LV2_Handle instance, uint32_t size,	const void* data)
	{
		if (*(const LV2WorkType*)data != kWorkTypeSwitch)
			return LV2_WORKER_ERR_UNKNOWN;

		auto msg = static_cast<const LV2SwitchModelMsg*>(data);
		auto nam = static_cast<NAM::Plugin*>(instance);


		// prepare reply for deleting old model
		LV2FreeModelMsg reply = { kWorkTypeFree, nam->currentModel };

		// swap current model with new one
		nam->currentModel = msg->model;
		if (nam->currentModel != nullptr)
		{
			nam->currentModelPath = msg->path;
		} else {
			nam->currentModelPath = "";
		}

		// make sure assignment didn't reallocate memory on RT thread.
		assert(nam->currentModelPath.capacity() >= MAX_FILE_NAME + 1); 

		// send reply
		nam->schedule->schedule_work(nam->schedule->handle, sizeof(reply), &reply);

		// report change to host/ui
		nam->write_current_path();

		return LV2_WORKER_SUCCESS;
	}

	void Plugin::process(uint32_t n_samples) noexcept
	{
		lv2_atom_forge_set_buffer(&atom_forge, (uint8_t*)ports.notify, ports.notify->atom.size);
		lv2_atom_forge_sequence_head(&atom_forge, &sequence_frame, uris.units_frame);

		if (requestModelPathUpdate)
		{
			requestModelPathUpdate = false;
			write_current_path();
		}

		LV2_ATOM_SEQUENCE_FOREACH(ports.control, event)
		{
			if (event->body.type == uris.atom_Object)
			{
				const auto obj = reinterpret_cast<LV2_Atom_Object*>(&event->body);
				if (obj->body.otype == uris.patch_Get)
				{
					write_current_path();
				}
				else if (obj->body.otype == uris.patch_Set)
				{
					const LV2_Atom* property = NULL;
					const LV2_Atom* file_path = NULL;

					lv2_atom_object_get(obj,
					                    uris.patch_property, &property,
					                    uris.patch_value, &file_path,
					                    0);

					if (property && property->type == uris.atom_URID &&
						((const LV2_Atom_URID*)property)->body == uris.model_Path &&
						file_path && file_path->type == uris.atom_Path &&
						file_path->size > 0 && file_path->size < MAX_FILE_NAME)
					{
						LV2LoadModelMsg msg = { kWorkTypeLoad, {} };
						memcpy(msg.path, file_path + 1, file_path->size);
						schedule->schedule_work(schedule->handle, sizeof(msg), &msg);
					}
				}
			}
		}

		// convert input level from db
		float desiredInputLevel = powf(10, *(ports.input_level) * 0.05f);

		if (fabs(desiredInputLevel - inputLevel) > SMOOTH_EPSILON)
		{
			for (unsigned int i = 0; i < n_samples; i++)
			{
				// do very basic smoothing
				inputLevel = (.99f * inputLevel) + (.01f * desiredInputLevel);

				ports.audio_out[i] = ports.audio_in[i] * inputLevel;
			}
		}
		else
		{
			inputLevel = desiredInputLevel;

			for (unsigned int i = 0; i < n_samples; i++)
			{
				ports.audio_out[i] = ports.audio_in[i] * inputLevel;
			}
		}

		if (currentModel != nullptr)
		{
			currentModel->process(&ports.audio_out, &ports.audio_out, 1, n_samples, 1.0, 1.0, mNAMParams);
			currentModel->finalize_(n_samples);
		}

		// convert output level from db
		float desiredOutputLevel = powf(10, *(ports.output_level) * 0.05f);

		if (fabs(desiredOutputLevel - outputLevel) > SMOOTH_EPSILON)
		{
			for (unsigned int i = 0; i < n_samples; i++)
			{
				// do very basic smoothing
				outputLevel = (.99f * outputLevel) + (.01f * desiredOutputLevel);

				ports.audio_out[i] *= outputLevel;
			}
		}
		else
		{
			outputLevel = desiredOutputLevel;

			for (unsigned int i = 0; i < n_samples; i++)
			{
				ports.audio_out[i] *= outputLevel;
			}
		}
	}

	uint32_t Plugin::options_get(LV2_Handle, LV2_Options_Option*)
	{
		// currently unused
		return LV2_OPTIONS_ERR_UNKNOWN;
	}

	uint32_t Plugin::options_set(LV2_Handle instance, const LV2_Options_Option* options)
	{
		auto nam = static_cast<NAM::Plugin*>(instance);

		for (int i=0; options[i].key && options[i].type; ++i)
		{
			if (options[i].key == nam->uris.bufSize_maxBlockLength && options[i].type == nam->uris.atom_Int)
			{
				nam->maxBufferSize = *(const int32_t*)options[i].value;
				break;
			}
		}

		return LV2_OPTIONS_SUCCESS;
	}

	LV2_State_Status Plugin::save(LV2_Handle instance, LV2_State_Store_Function store, LV2_State_Handle handle, 
		uint32_t flags, const LV2_Feature* const* features)
	{
		auto nam = static_cast<NAM::Plugin*>(instance);

		lv2_log_trace(&nam->logger, "Saving state\n");

		if (!nam->currentModel) {
			return LV2_STATE_SUCCESS;
		}

		LV2_State_Map_Path* map_path = (LV2_State_Map_Path*)lv2_features_data(features, LV2_STATE__mapPath);

		if (map_path == nullptr)
		{
			lv2_log_error(&nam->logger, "LV2_STATE__mapPath unsupported by host\n");

			return LV2_STATE_ERR_NO_FEATURE;
		}

		// Map absolute sample path to an abstract state path
		char* apath = map_path->abstract_path(map_path->handle, nam->currentModelPath.c_str());

		store(handle, nam->uris.model_Path, apath, strlen(apath) + 1, nam->uris.atom_Path,
			LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

		LV2_State_Free_Path* free_path = (LV2_State_Free_Path *)lv2_features_data(features, LV2_STATE__freePath);

		if (free_path != nullptr)
		{
			free_path->free_path(free_path->handle, apath);
		}
		else
		{
#ifndef _WIN32	// Can't free host-allocated memory on plugin side under Windows
			free(apath);
#endif
		}

		return LV2_STATE_SUCCESS;
	}

	LV2_State_Status Plugin::restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle, 
		uint32_t flags, const LV2_Feature* const* features)
	{
		//if (!haveLog) return LV2_STATE_SUCCESS;

		auto nam = static_cast<NAM::Plugin*>(instance);

		// Get model_Path from state
		size_t      size     = 0;
		uint32_t    type     = 0;
		uint32_t    valflags = 0;
		const void* value = retrieve(handle, nam->uris.model_Path, &size, &type, &valflags);
		const char* path = "";
		char* pathMemory = nullptr;

		lv2_log_trace(&nam->logger, "Restoring model '%s'\n", (const char*)value);

		if (value) {
			if (type != nam->uris.atom_Path) {
				lv2_log_error(&nam->logger, "Non-path model_Path\n");
				return LV2_STATE_ERR_BAD_TYPE;
			}

			LV2_State_Map_Path* map_path = (LV2_State_Map_Path*)lv2_features_data(features, LV2_STATE__mapPath);

			if (map_path == nullptr)
			{
				lv2_log_error(&nam->logger, "LV2_STATE__mapPath unsupported by host\n");

				return LV2_STATE_ERR_NO_FEATURE;
			}

			// Map abstract state path to absolute path
			pathMemory = map_path->absolute_path(map_path->handle, (const char *)value);
			path = pathMemory;
		}

		size_t pathLen = strlen(path);

		LV2_State_Status result = LV2_STATE_SUCCESS;

		if (pathLen < MAX_FILE_NAME)
		{
			// Schedule model to be loaded by the provided worker
			NAM::LV2LoadModelMsg msg = { NAM::kWorkTypeLoad, {} };
			memcpy(msg.path, path, pathLen);

			if (!nam->activated)
			{
				// load the model synchronously.
				::DSP*model = nullptr;
				try {
					model = nam->load_model(msg.path);
					nam->currentModelPath = msg.path;
					nam->requestModelPathUpdate = true;

				} catch (const std::exception&e)
				{
					lv2_log_error(&nam->logger, "Unable to load model from: '%s'\n", msg.path);
				}
				nam->currentModel = model;
			} else {
				nam->schedule->schedule_work(nam->schedule->handle, sizeof(msg), &msg);
			}
		}
		else
		{
			lv2_log_error(&nam->logger, "Model path is too long (max %u chars)\n", MAX_FILE_NAME);

			result = LV2_STATE_ERR_UNKNOWN;
		}

		if (pathMemory)
		{
			LV2_State_Free_Path* free_path = (LV2_State_Free_Path*)lv2_features_data(features, LV2_STATE__freePath);

			if (free_path != nullptr)
			{
				free_path->free_path(free_path->handle, pathMemory);
			}
			else
			{
	#ifndef _WIN32	// Can't free host-allocated memory on plugin side under Windows. Just leak it.
				free(pathMemory);
	#endif
			}
		}

		return result;
	}

	void Plugin::write_current_path()
	{
		LV2_Atom_Forge_Frame frame;

		lv2_atom_forge_frame_time(&atom_forge, 0);
		lv2_atom_forge_object(&atom_forge, &frame, 0, uris.patch_Set);

		lv2_atom_forge_key(&atom_forge, uris.patch_property);
		lv2_atom_forge_urid(&atom_forge, uris.model_Path);
		lv2_atom_forge_key(&atom_forge, uris.patch_value);
		lv2_atom_forge_path(&atom_forge, currentModelPath.c_str(), currentModelPath.length() + 1);

		lv2_atom_forge_pop(&atom_forge, &frame);
	}

	void Plugin::activate()
	{
		activated = true;

		// notify of empty path if state_restore hasn't been called.
		requestModelPathUpdate = true; 
	}
	void Plugin::deactivate()
	{
		activated = false;
	}
}
