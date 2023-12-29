#include "tp.h"
#include "tp2.h"

#include "utils.h"

#include <filesystem>

#undef main

int main(int argc, char** argv)
{
    TP2 tp;
    tp.run();

    ImGui_ImplSdlGL3_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
