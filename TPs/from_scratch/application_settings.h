#ifndef APPLICATION_SETTINGS_H
#define APPLICATION_SETTINGS_H

#include "color.h"

#include <string>

struct ApplicationSettings
{
	bool enable_vsync = true;
    int gpu_frustum_culling = 1;
    bool draw_mesh_bboxes = false;

    bool use_irradiance_map = true;

	//1 for cubemap, 0 for skysphere
	int cubemap_or_skysphere = 0;

    float shadow_intensity = 0.1f;
    bool draw_shadow_map = false;

    float hdr_exposure = 2.1f;

    bool currently_recomputing_irradiance = false;

    /*int irradiance_map_precomputation_samples = 16384 * 32;
    int irradiance_map_precomputation_downscale_factor = 64;
    std::string irradiance_map_file_path = "../data/skyspheres/blaubeuren_night_8k.hdr";*/

    /*int irradiance_map_precomputation_samples = 16384;
    int irradiance_map_precomputation_downscale_factor = 64;
    std::string irradiance_map_file_path = "../data/skyspheres/the_sky_is_on_fire_8k.hdr";*/

//    int irradiance_map_precomputation_samples = 16384*32;
//    int irradiance_map_precomputation_downscale_factor = 64;
//    std::string irradiance_map_file_path = "../data/skyspheres/evening_road_01_puresky_2k.hdr";

//    int irradiance_map_precomputation_samples = 16384 * 4;
//    int irradiance_map_precomputation_downscale_factor = 64;
//    std::string irradiance_map_file_path = "../data/skyspheres/above_clouds_4k.hdr";

    int irradiance_map_precomputation_samples = 16384 / 4;
    int irradiance_map_precomputation_downscale_factor = 1;
    std::string irradiance_map_file_path = "../data/skyspheres/above_clouds_4k.hdr";

    //Whether or not to use the metalness and roughness here for the material shading
    //If true, these values are going to be used. If false, the values found in
    //the metalness / roughness maps will be used
    bool override_material = true;
	float mesh_roughness = 0.3f;
    float mesh_metalness = 1.0f;
    bool do_normal_mapping = 1;
	Color ambient_color = Color(0.1, 0.1, 0.1, 0);
};

#endif
