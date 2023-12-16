#ifndef APPLICATION_SETTINGS_H
#define APPLICATION_SETTINGS_H

#include "color.h"

#include <string>

struct ApplicationSettings
{
	bool enable_vsync = true;

    bool use_irradiance_map = true;

	//1 for cubemap, 0 for skysphere
	int cubemap_or_skysphere = 0;

    float shadow_intensity = 0.3;
    bool draw_shadow_map = false;
    bool bind_light_camera_to_camera = false;
    bool draw_light_camera_frustum = false;

    float hdr_exposure = 0.9f;

	bool currently_recomputing_irradiance = false;

    int irradiance_map_precomputation_samples = 16384 * 4;
    int irradiance_map_precomputation_downscale_factor = 16;
    std::string irradiance_map_file_path = "data/TPs/evening_road_01_puresky_2k.hdr";

//    int irradiance_map_precomputation_samples = 16384 * 4;
//    int irradiance_map_precomputation_downscale_factor = 16;
//    std::string irradiance_map_file_path = "data/TPs/the_sky_is_on_fire_4k.hdr";

//    int irradiance_map_precomputation_samples = 16384 * 128;
//    int irradiance_map_precomputation_downscale_factor = 32;
//    std::string irradiance_map_file_path = "data/TPs/blaubeuren_night_4k.hdr";

    //Whether or not to use the metalness and roughness here for the material shading
    //If true, these values are going to be used. If false, the values found in
    //the metalness / roughness maps will be used
    bool override_material = false;
	float mesh_roughness = 0.3f;
    float mesh_metalness = 1.0f;
	Color ambient_color = Color(0.1, 0.1, 0.1, 0);
};

#endif
