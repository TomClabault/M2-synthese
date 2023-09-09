#include "tp.h"

#include "utils.h"

int main(int argc, char** argv)
{
	//Image irradiance_map = Utils::precompute_irradiance_map_from_skysphere("TPs/from_scratch/data/AllSkyFree_Sky_EpicGloriousPink_Equirect.jpg", 384);
	//Image irradiance_map = Utils::precompute_irradiance_map_from_skysphere("TPs/from_scratch/data/blaubeuren_night_4k.png", 10);
	///Image irradiance_map = Utils::precompute_irradiance_map_from_skysphere("TPs/from_scratch/data/blaubeuren_night.jpg", 10);
	//Image irradiance_map = Utils::precompute_irradiance_map_from_skysphere("TPs/from_scratch/data/Capture.PNG", 10);
	//write_image(irradiance_map, "irradiance_map.png");
	//return 0;

	// il ne reste plus qu'a creer un objet application et la lancer 
	TP tp;
	tp.run();

	ImGui_ImplSdlGL3_Shutdown();
	ImGui::DestroyContext();

	return 0;
}