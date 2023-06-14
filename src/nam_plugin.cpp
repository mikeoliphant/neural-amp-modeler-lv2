#include <algorithm>
#include <cmath>
#include <utility>

#include "nam_plugin.h"
#include "activations.h"
#include <cassert>
#include <filesystem>

#define SMOOTH_EPSILON .0001f

namespace NAM {
	Plugin::Plugin()
	{
		// Turn on fast tanh approximation
		activations::Activation::enable_fast_tanh();

		// prevent allocations on the audio thread
		currentModelPath.reserve(MAX_FILE_NAME+1);
	}

	bool Plugin::initialize(double rate, const LV2_Feature* const* features) noexcept
	{
		
		logger.log = nullptr;

		for (size_t i = 0; features[i]; ++i) {
			if (std::string(features[i]->URI) == std::string(LV2_URID__map))
				map = static_cast<LV2_URID_Map*>(features[i]->data);
			else if (std::string(features[i]->URI) == std::string(LV2_WORKER__schedule))
				schedule = static_cast<LV2_Worker_Schedule*>(features[i]->data);
			else if (std::string(features[i]->URI) == std::string(LV2_LOG__log))
				logger.log = static_cast<LV2_Log_Log*>(features[i]->data);
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
		uris.patch_Set = map->map(map->handle, LV2_PATCH__Set);
		uris.patch_Get = map->map(map->handle, LV2_PATCH__Get);
		uris.patch_property = map->map(map->handle, LV2_PATCH__property);
		uris.patch_value = map->map(map->handle, LV2_PATCH__value);
		uris.units_frame = map->map(map->handle, LV2_UNITS__frame);
		uris.state_StateChanged = map->map(map->handle, LV2_STATE__StateChanged);

		uris.model_Path = map->map(map->handle, MODEL_URI);

		// MOD requires property notification without asking when started.
		requestPathPropertyNotification = true; 


		return true;
	}

	LV2_Worker_Status Plugin::work(LV2_Handle instance, LV2_Worker_Respond_Function respond, LV2_Worker_Respond_Handle handle,
		uint32_t size, const void* data)
	{
		switch (*((const uint32_t*)data))
		{
			case kWorkTypeLoad:
				auto msg = reinterpret_cast<const LV2LoadModelMsg*>(data);
				auto nam = static_cast<NAM::Plugin*>(instance);

				try
				{
					// If we had a previous model, delete it
					if (nam->deleteModel)
					{
						nam->deleteModel.reset();
					}

					if (strlen(msg->path) == 0)
					{	
						// avoid logging an error on an empty path.
						// but do clear the model.
						nam->stagedModel = nullptr; 
						nam->stagedModelPath = msg->path;
					} else 
					{
						lv2_log_trace(&nam->logger, "Staging model change: `%s`\n", msg->path);

						nam->stagedModel = get_dsp(msg->path);
						nam->stagedModelPath = msg->path;

						// Enable model loudness normalization
						nam->stagedModel->SetNormalize(true);
					}

					LV2WorkType response = kWorkTypeSwitch;
					respond(handle, sizeof(response), &response);

					return LV2_WORKER_SUCCESS;
				}
				catch (std::exception& e)
				{
					lv2_log_error(&nam->logger, "Unable to load model from: '%s'\n", msg->path);
				}

				break;
		}

		return LV2_WORKER_ERR_UNKNOWN;
	}

	LV2_Worker_Status Plugin::work_response(LV2_Handle instance, uint32_t size,	const void* data)
	{
		switch (*((const uint32_t*)data))
		{
		case kWorkTypeSwitch:
			try
			{
				auto nam = static_cast<NAM::Plugin*>(instance);

				std::swap(nam->currentModel, nam->stagedModel);
				
				nam->deleteModel = std::move(nam->stagedModel);

				return LV2_WORKER_SUCCESS;
			}
			catch (std::exception& e)
			{
			}

			break;
		}

		return LV2_WORKER_ERR_UNKNOWN;
	}


	void Plugin::process(uint32_t n_samples) noexcept
	{
		lv2_atom_forge_set_buffer(&atom_forge,(uint8_t*)ports.notify,ports.notify->atom.size);
		lv2_atom_forge_sequence_head(&atom_forge,&sequence_frame,uris.units_frame);

		LV2_ATOM_SEQUENCE_FOREACH(ports.control, event)
		{
			if (event->body.type == uris.atom_Object)
			{
				const auto obj = reinterpret_cast<LV2_Atom_Object*>(&event->body);
				if (obj->body.otype == uris.patch_Get) {
					this->requestPathPropertyNotification = true;
				}

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

							if (file_path && (file_path->size > 0) && (file_path->size < MaximumFileLength))
							{
								request_work((const char*)LV2_ATOM_BODY_CONST(file_path), file_path->size);
								this->requestStateChangeNotification = true;
							}
						}
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
		if (this->requestPathPropertyNotification)
		{
			requestPathPropertyNotification = false;
			lv2_atom_forge_frame_time(&atom_forge, 0);
			write_set_patch(currentModelPath);

		}
		if (requestStateChangeNotification)
		{
			requestStateChangeNotification = false;
			lv2_atom_forge_frame_time(&atom_forge, 0);
			write_state_changed();
		}

		lv2_atom_forge_pop(&atom_forge,&sequence_frame);
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
#ifndef _WIN32	// Can't free library allocated memory on Windows
			free(apath);
#endif
		}

		return LV2_STATE_SUCCESS;
	}

	static bool isAbsolutePath(const char*path)
	{
		// Warning: not for use on realtime thread, since it allocates memory.
		// This method will need to be replaced if the plugin ever implements threadSafeRestore.
		std::filesystem::path fsPath {path};
		return fsPath.is_absolute();
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

		lv2_log_trace(&nam->logger, "Restoring model '%s'\n", (const char*)value);

		if (!value) {
			lv2_log_error(&nam->logger, "Missing model_Path\n");
			return LV2_STATE_ERR_NO_PROPERTY;
		}

		if (type != nam->uris.atom_Path) {
			lv2_log_error(&nam->logger, "Non-path model_Path\n");
			return LV2_STATE_ERR_BAD_TYPE;
		}

		LV2_State_Map_Path* map_path = (LV2_State_Map_Path*)lv2_features_data(features, LV2_STATE__mapPath);

		char*mappedPath = nullptr;
		const char*path = (const char*)value;
		size_t pathLen = strlen(path);

		if (map_path != nullptr && pathLen != 0 && !isAbsolutePath(path))
		{
			// Map abstract state path to absolute path
			mappedPath = map_path->absolute_path(map_path->handle, (const char *)value);
			path = mappedPath;
		}


		LV2_State_Status result = LV2_STATE_SUCCESS;

		if (pathLen < MaximumFileLength)
		{
			// Schedule model to be loaded by the provided worker
			nam->request_work(path,pathLen);

			// MOD requires notification of the change of property after a state load.
			// The notification will get sent during the next run call.
			nam->requestPathPropertyNotification = true;
			nam->requestStateChangeNotification = false;
		}
		else
		{
			lv2_log_error(&nam->logger, "Model path is too long (max %d chars)\n",(int)MaximumFileLength);

			result = LV2_STATE_ERR_UNKNOWN;
		}

		if (mappedPath != nullptr)
		{
			LV2_State_Free_Path* free_path = (LV2_State_Free_Path*)lv2_features_data(features, LV2_STATE__freePath);

			if (free_path != nullptr)
			{
				free_path->free_path(free_path->handle, mappedPath);
			}
			else
			{
	#ifndef _WIN32	// Can't free library allocated memory on Windows, so just leak it.
				free(mappedPath);
	#endif
			}
		}
		// MOD isn't smart enough to do a PATCH_get after loading 
		// state, so send a PATCH_set without being asked when we next get the chance.
		nam->requestPathPropertyNotification = true;  
		return result;
	}

	void Plugin::write_set_patch( const std::string& filename)
	{
		LV2_Atom_Forge_Frame frame;

		lv2_atom_forge_object(&atom_forge, &frame, 0, uris.patch_Set);

		lv2_atom_forge_key(&atom_forge, uris.patch_property);
		lv2_atom_forge_urid(&atom_forge, uris.model_Path);
		lv2_atom_forge_key(&atom_forge, uris.patch_value);
		lv2_atom_forge_path(&atom_forge, filename.c_str(), filename.length());

		lv2_atom_forge_pop(&atom_forge, &frame);
	}

	void Plugin::write_state_changed()
	{
		LV2_Atom_Forge_Frame frame;

		lv2_atom_forge_object(&atom_forge, &frame, 0, uris.state_StateChanged);
			/* object with no properties */
		lv2_atom_forge_pop(&atom_forge, &frame);
	}

	void Plugin::request_work(const char*fileName, size_t length)
	{
		LV2LoadModelMsg msg = { kWorkTypeLoad, {} };
		memcpy(msg.path, fileName,length);
		this->currentModelPath = msg.path;
		
		schedule->schedule_work(schedule->handle, sizeof(msg), &msg);
	}

}
