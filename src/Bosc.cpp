#include <iostream>
#include <fstream>
#include "Bosc.h"

fs::path Bosc::_repos;
Bosc::Compiler Bosc::_compiler;
Bosc::Export Bosc::_export;


int Bosc::load_bruc() {
	std::string bruc_file = fs::path(dir_local / "bosc.bruc").string();

	b = Bruc::readFile(bruc_file);
	if (b.error()) {
		std::cerr << "Error: Couldn't process file " << bruc_file << std::endl;
		std::cerr << "At line " << b.getErrorLine() << ": " << b.error().message() << std::endl;
		return 1;
	}
	return 0;
}


fs::path Bosc::make_absolute(fs::path p) {
	if (!p.is_absolute()) p = dir_local / p;
	return p.lexically_normal();
}

int Bosc::create_dir(fs::path dir) {
	if (std::filesystem::exists(dir)) return 0;
	if (std::filesystem::create_directories(dir)) return 0;
	std::cerr << "Error: Couldn't create directory " << dir.string() << std::endl;
	return 1;
}

int Bosc::get_dep_path(fs::path& path, std::string name) {
	std::vector<std::string> dep_line = b.get<std::vector<std::string>>("depend", name);
	if(b.error()) {
		std::cerr << "Error: Couldn't process dependency " << name;
		std::cerr << " (" << b.error().message() << ")\n";
		return 1;
	}
	fs::path depend_path;
	if (dep_line[0] == "path") path = make_absolute(fs::path(dep_line[1]));
	else {
		path = _repos / name;
		if (!std::filesystem::exists(path)) {
			std::cout << "- Cloning repository: " << dep_line[1] << "\n";
			std::string cmd = "git clone " + dep_line[1] + " " + path.string();
			if (std::system(cmd.c_str())) {
				std::cerr << "Error: Couldn't clone repository\n";
				return 1;
			}
		}
	}
	return 0;
}

int Bosc::build_dependency(std::string name) {
	fs::path dir; 
	if (get_dep_path(dir, name)) return 1;

	Bosc child(dir, this);
	if (child.build(false)) return 1;

	return 0;
}

bool Bosc::is_older(fs::path p1, fs::path p2) {
	if (!std::filesystem::exists(p2)) {
		std::cerr << "Warning: File " << p2.string() << " doesn't exist\n";
	}
	if (!std::filesystem::exists(p1)) return true;

	auto t1 = std::filesystem::last_write_time(p1);
	auto t2 = std::filesystem::last_write_time(p2);

	return t1 < t2;
}


Bosc::Bosc(fs::path dir, Bosc* parent) : dir_local(dir), is_root(parent == nullptr) {
	if (!is_root) {
		depend_flags_parent = parent->depend_flags_local;
	} 

	depend_flags_local = depend_flags_parent;

}

int Bosc::build(bool install) {
	if (load_bruc()) return 1;

	// Set compiler and create the repo directory
	if (is_root) { 
		_repos = make_absolute(b.get<fs::path>("dirs", "repos", ".bosc"));
		if (create_dir(_repos)) return 1;
		_compiler.path = b.get<fs::path>("compiler", "path");
		_compiler.prefix = b.get("compiler", "prefix");
		_compiler.flags = b.get("compiler", "flags");
	}

	// Dependencies
	std::vector<std::string> deps = b.getKeys("depend");
	for (auto& d : deps) {
		if (build_dependency(d)) return 1;
	}

	bool prj_printed = false;
	// Build
	fs::path dir_build = make_absolute(b.get<fs::path>("dirs", "build", "build"));
	fs::path dir_objs = dir_build / "obj";
	if (create_dir(dir_objs)) return 1;

	std::string flags = _compiler.flags + " " + depend_flags_parent + " " + b.get("project","flags") + " " + _export.flags;
	std::string gcc = (_compiler.path / (_compiler.prefix + "gcc")).string();
	std::string gpp = (_compiler.path / (_compiler.prefix + "g++")).string();
	std::string incl = "";

	bool skip_link = true;

	for (auto i : _export.includes) incl += " -I"+i.string();
	for (auto i : b.get<fs::list>("project", "includes")) incl += " -I" + make_absolute(i).string();
	for (auto s : b.get<fs::list>("project", "sources")) {
		fs::path obj = dir_objs / s.stem().concat(".o");
		if (is_older(obj,make_absolute(s))) {
			std::string cmd;
			if (s.extension() == ".c" || s.extension() == ".S") cmd = gcc;
			else cmd = gpp;
			cmd += " " + flags + " " + incl + " -c " + make_absolute(s).string() + " -o " + obj.string();
			if (!prj_printed) {
				std::cout << "\n[" << b.get("project", "name", "UNKNOWN") << "]" << std::endl;
				prj_printed = true;
			}
			std::cout << "- Building " << s.filename().stem().string() << "\n";
			if(std::system(cmd.c_str())) return 1;
			skip_link = false;
		}
	}
	std::string objs = (dir_objs / "*").string();
	fs::path dir_targets = dir_build / "targets";
	if (create_dir(dir_targets)) return 1;
	std::string msg;
	for (auto& t : b.get<fs::list>("project", "targets")) { // We don't want the absolute path
		if (t.is_absolute()) {
			std::cerr << "Error: Targets must be in a relative path\n";
			return 1;
		}
		fs::path target = dir_targets / t;
		if (create_dir(target.parent_path()) ) return 1;
		std::string cmd;
		if (target.extension() == ".a") { //is a lib
			cmd = (_compiler.path / (_compiler.prefix + "ar")).string();
			cmd += " rcs " + target.string() + " " + objs;
			_export.lpaths.push_back(target.parent_path());
			_export.lnames.push_back(b.get("project", "name", "UNKNOWN"));
			msg = "- Packing " + b.get("project", "name", "UNKNOWN");
		} else {
			std::string link = b.get("project", "link");
			for (auto l : _export.lpaths) link += " -L" + l.string();
			for (auto l : _export.lnames) link += " -l" + l;
			cmd = gpp + " " + flags + " " + objs + " " + link + " " + " -o " + target.string();
			msg = "- Linking " + target.filename().stem().string();
		}
		if (std::filesystem::exists(target) && skip_link) continue;
		if (!prj_printed) {
			std::cout << "\n[" << b.get("project", "name", "UNKNOWN") << "]" << std::endl;
			prj_printed = true;
		}
		std::cout << msg << std::endl;
		if (std::system(cmd.c_str())) return 1;
	}
	_export.flags += b.get("export", "flags");
	for (auto p : b.get<fs::list>("export", "includes")) _export.includes.push_back(make_absolute(p));

	// Install
	if (!install) return 0;
	std::string name = b.get("project", "name", "UNKNOWN");
	fs::path dir_install = make_absolute(b.get<fs::path>("dirs", "install", "/opt") / name);
	std::string cmd = "rsync -a " + dir_targets.string() + "/ " + dir_install.string();
	if (!prj_printed) {
		std::cout << "\n[" << b.get("project", "name", "UNKNOWN") << "]" << std::endl;
		prj_printed = true;
	}
	std::cout << "- Installing\n";
	if (create_dir(dir_install)) return 1;
	if (std::system(cmd.c_str())) return 1;

	return 0;
}


int Bosc::clean(bool recursive) {
	if (load_bruc()) return 1;
	if (is_root) { 
		_repos = make_absolute(b.get<fs::path>("dirs", "repos", ".bosc"));
	}

	if (recursive) {
		std::vector<std::string> deps = b.getKeys("depend");
		for (auto& d : deps) {
			fs::path dir; 
			if (get_dep_path(dir, d)) return 1;
			Bosc child(dir, this);
			if(child.clean(true)) return 1;
		}

	}
	std::cout << "\n[" << b.get("project", "name", "UNKNOWN") << "]" << std::endl;
	fs::path dir_build = make_absolute(b.get<fs::path>("dirs", "build", "build"));
	std::string cmd = "rm -rf " + dir_build.string();
	std::cout << "- Cleaning\n";
	if(std::system(cmd.c_str())) return 1;
	return 0;
}


int Bosc::uninstall() {
	if (load_bruc()) return 1;
	std::string name = b.get("project", "name", "UNKNOWN");
	fs::path dir_install = make_absolute(b.get<fs::path>("dirs", "install", "/opt")) / name;
	std::string cmd = "rm -rf " + dir_install.string();
	std::cout << "\n[" << b.get("project", "name", "UNKNOWN") << "]" << std::endl;
	std::cout << "- Uninstalling\n";
	if (std::system(cmd.c_str())) return 1;
	return 0;
}

int Bosc::purge() {
	if(clean(true)) return 1;

	std::string name = b.get("project", "name", "UNKNOWN");
	fs::path dir_install = make_absolute(b.get<fs::path>("dirs", "install", "/opt")) / name;
	std::string cmd = "rm -rf " + dir_install.string();
	std::cout << "- Uninstalling\n";
	if (std::system(cmd.c_str())) return 1;

	cmd = "rm -rf " + _repos.string();
	std::cout << "- Purging deps\n";
	if (std::system(cmd.c_str())) return 1;
	return 0;
}
