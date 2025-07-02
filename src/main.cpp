#include <iostream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <string>

#include "Bosc.h"

#define launch(exp, txt) do { if (exp) { std::cout << std::endl << txt << " failed\n"; return 1; } std::cout << std::endl << txt << " completed\n"; } while (0)

int main(int argc, char* argv[]) {
    std::vector<std::string> valid_cmds = {"build", "install", "clean", "clean-all", "uninstall", "purge"};
    if (argc < 2) {
        std::cerr << "Expected command: ";
	for (size_t i = 0; i < valid_cmds.size(); ++i) {
		if (i > 0) std::cerr << " | ";
		std::cerr << valid_cmds[i];
	}
	std::cerr << std::endl;
        return 1;
    }

    std::string command = argv[1];
    if (std::find(valid_cmds.begin(), valid_cmds.end(), command) == valid_cmds.end()) {
        std::cerr << "Invalid command\n";
        return 1;
    }

    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    auto abs_path = std::filesystem::absolute(".").lexically_normal();
    Bosc app(abs_path, nullptr);

    if (command == "build") launch(app.build(false), "Build");
    else if (command == "install") launch(app.build(true), "Install");
    else if (command == "clean") launch(app.clean(false), "Clean");
    else if (command == "clean-all") launch(app.clean(true), "Clean-all");
    else if (command == "uninstall") launch(app.uninstall(), "Uninstall");
    else if (command == "purge") launch(app.purge(), "Purge");
    return 0;
}

