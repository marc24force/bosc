#include <iostream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <string>

#include "Bosc.h"

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

    if (command == "build") {
	    app.init();
	    app.depend();
	    app.build();
	    std::cout << "Build complete\n";
    } else if (command == "install") {
	    std::cout << "Install not implemneted\n";
	    //app.build() //do some conditional
	    //app.install();
	    std::cout << "Install complete\n";
    } else if (command == "clean") {
	    app.clean();
	    std::cout << "Project cleaned\n";
    } else if (command == "clean-all") {
	    app.clean_deps();
	    std::cout << "Project and subprojects cleaned\n";
    } else if (command == "uninstall") {
	    std::cout << "Project uninstalled\n";
    } else if (command == "purge") {
	    //app.clean_deps();
	    //app.clean();
	    //app.purge();
	    std::cout << "Project purged\n";
    }
    


    return 0;
}

