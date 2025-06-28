#include <iostream>
#include "Bosc.h"

fs::path Bosc::dir_repos;
fs::path Bosc::compiler_path; 
std::string Bosc::compiler_prefix;
std::string Bosc::compiler_flags;
std::string Bosc::export_flags;
fs::list Bosc::export_includes;
fs::list Bosc::export_libs_path;
std::vector<std::string> Bosc::export_libs_name;


Bosc::Bosc(fs::path local_dir, Bosc* parent) {
	std::string bruc_file = fs::path(local_dir / "bosc.bruc").string();
	std::cout << "Reading " << bruc_file << std::endl;

	conf = Bruc::readFile(bruc_file);
	if (conf.error()) {
		std::cerr << "ERROR! Couldn't process file " << bruc_file << std::endl;
		exit(1);
	}

	dir_local = local_dir;
	if (parent != nullptr) {
		depend_flags_parent = parent->depend_flags_local;
		depend_flags_local = depend_flags_parent;
	}
}

Bosc::~Bosc() {}


fs::path Bosc::getAbsPath(fs::path p) {
	if (!p.is_absolute()) p = dir_local / p;
	return p;
}


void Bosc::init() {
	dir_repos = getAbsPath(conf.get<fs::path>("dirs", "repos", ".bosc"));
	std::filesystem::create_directories(dir_repos);
	
	// Compiler section
	compiler_path = conf.get<fs::path>("compiler", "path");
	compiler_prefix = conf.get("compiler", "prefix");
	compiler_flags = conf.get("compiler", "flags");

}

void Bosc::depend() {

	// Run pre depend script
	fs::list scripts = conf.get<fs::list>("scripts", "pre_depend");
	for (auto s : scripts) {
		std::cout << "Running pre dependency script " << s << "\n";
		if(std::system(getAbsPath(s).string().c_str())) exit(1);
	}

	// If no depend section skip this
	if (!conf.exists("depend")) return;
	depend_flags_local += conf.get("depend", "flags");

	std::vector<std::string> dependencies = conf.getKeys("depend");
	for ( auto depend_name : dependencies) {
		std::cout << "Resolving dependency " << depend_name << "\n";
		std::vector<std::string> depend_cmd = conf.get<std::vector<std::string>>("depend", depend_name);
		fs::path depend_path;
		if (depend_cmd[0] == "path") {
			depend_path = getAbsPath(fs::path(depend_cmd[1]));
		} else {
			depend_path = dir_repos / depend_name;
			if (!std::filesystem::exists(depend_path)) {
				std::cout << "- Cloning repository: " << depend_cmd[1] << "\n";
				std::string cmd = "git clone " + depend_cmd[1] + " " + depend_path.string();
				if (std::system(cmd.c_str())) exit(1);
			}
		}
		Bosc child(depend_path, this);
		child.depend();
		child.build();
	}

}

void Bosc::build() {
	// Create build directory
	fs::path dir_build = getAbsPath(conf.get<fs::path>("dirs", "build", "build"));
	fs::path dir_objs = dir_build / "obj";
	std::filesystem::create_directories(dir_objs);
	
	// Run pre build script
	fs::list scripts = conf.get<fs::list>("scripts", "pre_build");
	for (auto s : scripts) {
		std::cout << "Running pre build script " << s << "\n";
		if(std::system(getAbsPath(s).string().c_str())) exit(1);
	}

	std::cout << "Starting build of " << conf.get("project", "name", "UNKNOWN") << "\n";

	std::string flags = compiler_flags + " " + depend_flags_parent + " " + conf.get("project","flags") + " " + export_flags;
	std::string gcc = (compiler_path / (compiler_prefix + "gcc")).string();
	std::string gpp = (compiler_path / (compiler_prefix + "g++")).string();
	std::string incl = "";
	for (auto i : export_includes) incl += " -I"+i.string();
	for (auto i : conf.get<fs::list>("project", "includes")) incl += " -I"+getAbsPath(i).string();
	for (auto s : conf.get<fs::list>("project", "sources")) {
		fs::path obj =  dir_objs / s.stem().concat(".o");
		if (std::filesystem::exists(obj)) continue;
		std::string cmd;
		if (s.extension() == ".c" || s.extension() == ".S") cmd = gcc;
		else cmd = gpp;

		cmd += " " + flags + " " + incl + " -c " + getAbsPath(s).string() + " -o " + obj.string();
		std::cout << "- Compiling " << s.string() << "\n";
		if(std::system(cmd.c_str())) exit(1);
	}
	std::string objs = (dir_objs / "*").string();

	fs::path dir_targets = dir_build / "targets";
	std::filesystem::create_directories(dir_targets);

	for (auto t : conf.get<fs::list>("project", "targets")) {
		if (t.is_absolute()) {
			std::cerr << "ERROR! Target must be in a relative path\n";
			exit(1);
		}
		fs::path target = dir_targets / t;
		if (std::filesystem::exists(target)) continue;
		std::filesystem::create_directories(target.parent_path());
		std::cout << "- Generating " << target.string() << "\n";
		if (target.extension() == ".a") { //is a lib
			std::string cmd;
			cmd = (compiler_path / (compiler_prefix + "ar")).string();
			cmd += " rcs " + target.string() + " " + objs;
			if(std::system(cmd.c_str())) exit(1);

			export_libs_path.push_back(target.parent_path());
			export_libs_name.push_back(conf.get("project", "name", "UNKNOWN"));
		} else {
			std::string cmd;
			std::string link = conf.get("project", "link");
			for (auto l : export_libs_path) link += " -L" + l.string();
			for (auto l : export_libs_name) link += " -l" + l;
			cmd = gpp + " " + flags + " " + objs + " " + link + " " + " -o " + target.string();
			if(std::system(cmd.c_str())) exit(1);

		}

	}

	
	// Run post build script
	scripts = conf.get<fs::list>("scripts", "post_build");
	for (auto s : scripts) {
		std::cout << "Running post build script " << s << "\n";
		if(std::system(getAbsPath(s).string().c_str())) exit(1);
	}
	
	// Once build, set export flags
	export_flags += conf.get("export", "flags");
	fs::list tmp = conf.get<fs::list>("export", "includes");
	for (auto p : tmp) export_includes.push_back(getAbsPath(p));
}

void Bosc::clean() {
	fs::path dir_build = getAbsPath(conf.get<fs::path>("dirs", "build", "build"));
	std::string cmd = "rm -rf " + dir_build.string();
	if(std::system(cmd.c_str())) exit(1);
}

void Bosc::clean_deps() {
	dir_repos = getAbsPath(conf.get<fs::path>("dirs", "repos", ".bosc"));
	std::vector<std::string> dependencies = conf.getKeys("depend");
	for ( auto depend_name : dependencies) {
		std::vector<std::string> depend_cmd = conf.get<std::vector<std::string>>("depend", depend_name);
		fs::path depend_path;
		if (depend_cmd[0] == "path") {
			depend_path = getAbsPath(fs::path(depend_cmd[1]));
		} else {
			depend_path = dir_repos / depend_name;
		}
		Bosc child(depend_path, this);
		child.clean_deps();
	}
	clean();
}
