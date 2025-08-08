#include "Bosc.h"
#include <vector>
#include <iostream>

void print_usage(const std::string& prog_name) {
	std::cout << "Usage:\n"
		<< "  " << prog_name << " [flags] <cmd> [args]\n\n"
		<< "Commands:\n"
		<< "  import        Download dependencies\n"
		<< "  build         Build project and dependencies\n"
		<< "  install       Install project\n"
		<< "  uninstall     Uninstall project\n"
		<< "  clean         Clean current project build\n"
		<< "  purge         Clean all project builds\n"
		<< "  remove        Remove project entry (deletes repo if applicable)\n\n"
		<< "Flags:\n"
		<< "  -v, --verbose           Enable verbose output\n"
		<< "  -r, --recursive         Apply recursively (only for clean, purge, remove)\n"
		<< "  -p <project>, --project=<project>\n"
		<< "                          Target specific project (only for clean, purge, remove)\n"
		<< "  -h, --help              Show this message\n";
}

struct Options {
	bool verbose = false;
	bool recursive = false;
	std::string project;
	std::string cmd;
	std::vector<std::string> args;
};

std::string get_project_path(const std::string& name) {
	Ciri p = Bosc::projects();
	if (!p.HasValue(name, "path")) throw std::runtime_error("Project " + name + " does not exist or is not managed by Bosc");
	return p.GetString(name, "path", "");
}

Options parse(int argc, char* argv[]) {
	std::vector<std::string> v(argv, argv + argc);
	Options o;

	const std::set<std::string> valid_cmds = {
		"import", "build", "install", "uninstall", "clean", "purge", "remove"
	};

	o.project = "./";

	size_t i = 1;
	for (; i < v.size(); ++i) {
		const auto& arg = v[i];
		if (arg == "-v" || arg == "--verbose") {
			o.verbose = true;
		} else if (arg == "-r" || arg == "--recursive") {
			o.recursive = true;
		} else if (arg == "-h" || arg == "--help") {
			print_usage(v[0]);
			std::exit(0);
		} else if (arg == "-p") {
			if (i + 1 < v.size()) {
				o.project = get_project_path(v[++i]);
			} else {
				std::cerr << "Missing value for -p\n";
				print_usage(v[0]);
				std::exit(1);
			}
		} else if (arg.rfind("--project=", 0) == 0) {
			o.project = get_project_path(arg.substr(10));
		} else if (arg[0] == '-') {
			std::cerr << "Unknown flag: " << arg << "\n";
			print_usage(v[0]);
			std::exit(1);
		} else {
			o.cmd = arg;
			++i;
			break;
		}
	}

	if (!o.cmd.empty() && !valid_cmds.count(o.cmd)) {
		std::cerr << "Unknown command: " << o.cmd << "\n";
		print_usage(v[0]);
		std::exit(1);
	}

	for (; i < v.size(); ++i) {
		o.args.push_back(v[i]);
	}

	if (o.cmd.empty()) {
		std::cerr << "No command provided.\n";
		print_usage(v[0]);
		std::exit(1);
	}

	return o;
}


int main(int argc, char* argv[]) {
	try {
		auto opts = parse(argc, argv);
		Bosc bosc(opts.project, opts.verbose);

		if (opts.cmd == "import") {
			bosc.import();
		} else if (opts.cmd == "build") {
			bosc.import();
			bosc.build();
		} else if (opts.cmd == "install") {
			bosc.import();
			bosc.build();
			bosc.install();
		} else if (opts.cmd == "uninstall") {
			bosc.uninstall();
		} else if (opts.cmd == "clean") {
			if (opts.recursive) bosc.import();
			bosc.clean(opts.recursive);
		} else if (opts.cmd == "purge") {
			if (opts.recursive) bosc.import();
			bosc.purge(opts.recursive);
		} else if (opts.cmd == "remove") {
			if (opts.recursive) bosc.import();
			bosc.remove(opts.recursive);
		}
	} catch (const std::exception& e) {
		std::cerr << "\n-- ERROR -- \n" << e.what() << "\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

