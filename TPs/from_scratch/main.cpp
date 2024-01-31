#include "tp2.h"

#include "utils.h"

#include <filesystem>

void process_command_line_arguments(int argc, char** argv, CommandlineArguments& commandline_arguments)
{
    for (int i = 1; i < argc; i++)
    {
        std::string string_argv = std::string(argv[i]);

        if (string_argv.starts_with("--orbiter="))
            commandline_arguments.camera_orbiter_file_path = string_argv.substr(10);
        else
            //Assuming this is the obj file
            commandline_arguments.obj_file_path = string_argv;
    }
}

int main(int argc, char** argv)
{
    CommandlineArguments commandline_arguments;
    process_command_line_arguments(argc, argv, commandline_arguments);

    TP2 tp(commandline_arguments);
	tp.run();

    ImGui_ImplSdlGL3_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
