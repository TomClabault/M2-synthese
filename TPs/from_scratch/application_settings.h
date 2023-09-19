#ifndef APPLICATION_SETTINGS_H
#define APPLICATION_SETTINGS_H

#include "color.h"

struct ApplicationSettings
{
	bool enable_vsync = true;

	bool use_ambient = true;

	//1 for cubemap, 0 for skysphere
	int cubemap_or_skysphere = 0;

	bool currently_recomputing_irradiance = false;
	int irradiance_map_precomputation_samples = 20;
	int irradiance_map_precomputation_downscale_factor = 1;
    std::string irradiance_map_file_path = "data/TPs/AllSkyFree_Sky_EpicGloriousPink_Equirect.jpg";

	float mesh_roughness = 0.5f;
	Color ambient_color = Color(0.1, 0.1, 0.1, 0);
};

#endif
