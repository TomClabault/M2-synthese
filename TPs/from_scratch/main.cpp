#include "tp.h"
#include "tp2.h"

#include "utils.h"

#include <filesystem>

int main(int argc, char** argv)
{
    std::cout << std::filesystem::current_path();
	const int SAMPLES = 1;

	//Utils::precompute_irradiance_map_from_skysphere_and_write("TPs/from_scratch/data/AllSkyFree_Sky_EpicGloriousPink_Equirect.jpg", 
															  //SAMPLES, 
															  //"test.png"/*"TPs/from_scratch/data/AllSkyFree_Sky_EpicGloriousPink_EquirectIrradiance_testx.png"*/);

	//Utils::precompute_irradiance_map_from_skysphere_and_write("TPs/from_scratch/data/SkysphereDebug.jpg", 
															  //SAMPLES, 
															  //"test.png"/*"TPs/from_scratch/data/AllSkyFree_Sky_EpicGloriousPink_EquirectIrradiance_testx.png"*/);

	//return 0;

    TP2 tp;
	tp.run();

	ImGui_ImplSdlGL3_Shutdown();
	ImGui::DestroyContext();

	return 0;
}
