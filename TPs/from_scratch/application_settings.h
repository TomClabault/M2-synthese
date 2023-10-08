#ifndef APPLICATION_SETTINGS_H
#define APPLICATION_SETTINGS_H

#include "color.h"

#include <string>

struct ApplicationSettings
{
	bool enable_vsync = true;

	bool use_ambient = true;

	//1 for cubemap, 0 for skysphere
	int cubemap_or_skysphere = 0;

    bool draw_shadow_map = false;
    bool bind_light_camera_to_camera = false;
    bool draw_light_camera_frustum = false;

    float hdr_exposure = 2.0f;

	bool currently_recomputing_irradiance = false;
    int irradiance_map_precomputation_samples = 16384 * 4 * 4 * 4;
    int irradiance_map_precomputation_downscale_factor = 8;
    //std::string irradiance_map_file_path = "data/TPs/AllSkyFree_Sky_EpicGloriousPink_Equirect.jpg"; //Non HDR
    //std::string irradiance_map_file_path = "data/TPs/DaySkyHDRI015A_4K-HDR_blurred.hdr";
    //std::string irradiance_map_file_path = "data/TPs/blaubeuren_night_2k.hdr";
    //std::string irradiance_map_file_path = "data/TPs/industrial_sunset_puresky_4k.hdr";
    std::string irradiance_map_file_path = "data/TPs/evening_road_01_puresky_2k.hdr";

	float mesh_roughness = 0.5f;
	Color ambient_color = Color(0.1, 0.1, 0.1, 0);
};

#endif
