#include "Bosc.h"
#include <iostream>
#include <unistd.h>
#include <unordered_set>
#include <format>
#include <fstream>
#include <regex>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;


void create_file(const fs::path& file) {
	fs::create_directories(file.parent_path());
	std::ofstream touch(file, std::ios::app);
	if (!touch) throw std::ios_base::failure("Could not create or open '" + file.string() + "'");
}

void remove_path(const fs::path& p) {
	if (fs::remove_all(p) == 0) 
		throw std::runtime_error(p.string() + " does not exist or cannot be removed");
	
}

bool is_subdir(const fs::path& base, const fs::path& sub) {
    auto base_abs = fs::weakly_canonical(base);
    auto sub_abs  = fs::weakly_canonical(sub);
    return std::mismatch(base_abs.begin(), base_abs.end(), sub_abs.begin()).first == base_abs.end();
}

bool valid_obj(fs::path obj, fs::path src) {
	if (!fs::exists(src)) throw std::runtime_error("Input file '" + src.string() + "' does not exist");
	if (!fs::exists(obj)) return false;

	auto t1 = std::filesystem::last_write_time(obj);
	auto t2 = std::filesystem::last_write_time(src);

	return t1 > t2;
}

std::string get_hash(std::string compiler) {
	std::size_t h = std::hash<std::string>{}(compiler);
	uint32_t t = static_cast<uint32_t>(h);

	std::stringstream ss;
	ss << std::hex << std::setw(8) << std::setfill('0') << t;
	return ss.str();
}

std::string list_to_string(std::vector<std::string> flags, std::string prefix) {
	std::string ret = "";
	for (const auto& f : flags) ret += prefix + f + " ";
	return ret;
}

// Initialize _compiler
std::string Bosc::_compiler = "";
// Initialize _hash
std::string Bosc::_hash = "";

// Initialize projects file
Ciri Bosc::_projects([]{
		// Open the projects file
		std::string bsc = []{ 
			const char* v = std::getenv("BOSC_ROOT");
			return v ? (std::string(v) + "/.projects.ini") : "";
		}();
		std::string xdg = []{ 
			const char* v = std::getenv("XDG_DATA_HOME");
			return v ? (std::string(v) + "/bosc/.projects.ini") : "";
		}();
		std::string home = []{ 
			const char* v = std::getenv("HOME");
			return v ? (std::string(v) + "/.local/bosc/.projects.ini") : "";
		}();
		std::string bosc = (!bsc.empty() ? bsc : (!xdg.empty() ? xdg : home));
		create_file(fs::path(bosc));
		return bosc;
		}()) ;

// Initialize init boolean
bool Bosc::_root = true;

std::string Bosc::_namespace = "";

Ciri& Bosc::projects() {
	return _projects;
}

Bosc::Bosc(const std::string& dir, bool verbose) : _name(fs::canonical(dir).filename()), _verbose(verbose), _config(fs::canonical(dir) / "bosc.ini") {
	// Check errors in the _config loading
	if (_config.ParseError() < 0) throw std::ios_base::failure("Could not open '" + dir + "/bosc.ini'");
	else if (_config.ParseError() > 0) throw std::runtime_error("Error parsing '" + dir + "/bosc.ini' at line " + std::to_string(_config.ParseError()));

	if (bool tmp = _root; _root = false, tmp) {
		// Load the compiler info
		_compiler = (fs::path(_config.GetString("compiler", "path", "")) / (_config.GetString("compiler", "prefix", "") + "{} ")).string() + list_to_string(_config.GetStringList("compiler", "flags", {}), "");
		_hash = get_hash(_compiler);
	}
}

void Bosc::import() {

	// For each entry in import
	for (const auto& key : _config.Keys("import")) {
		auto entry = _config.GetList("import", key, {});
		// Check the entry is well formated
		if (entry.size() < 2) throw std::runtime_error("Wrong format for import '" + key + "'");

		if(_projects.HasValue(key, "path")) { // Project already downloaded
			if (_verbose) {
				if (_namespace != _name) {
					std::cout << "\n[" << _name << "]" << "\n";
					_namespace = _name;
				}
				std::cout << "Import '" + key + "' found in '" + _projects.GetString(key, "path", "") + "'\n";
			}
			continue;
		}

		const auto& type = entry[0];
		const auto& target = entry[1];
		std::string version = entry.size() > 2 ? entry[2] : "";
		std::string target_ini = entry.size() > 3 ? entry[3] : "";

		fs::path dir;

		if (type == "repo") { // import is a repo
			dir = fs::path(_projects.File()).parent_path() / key;

			if (!fs::exists(dir)) {
				if (_namespace != _name) {
					std::cout << "\n[" << _name << "]" << "\n";
					_namespace = _name;
				}
				std::cout << " - Downloading " << key << "\n";

				std::string cmd = "git clone " + std::string(_verbose ? "" : "-q ") + target + " " + dir.string();
				// If a version is passed checkout to that
				if (!version.empty()) {
					std::string quiet = _verbose ? "" : " > /dev/null 2>&1";
					cmd += " && cd " + dir.string() + quiet + " && git checkout " + version + quiet;
				}
				// Run the cmd and handle error
				int res = std::system(cmd.c_str());
				if (res != 0) throw std::runtime_error("Git clone failed for " + key);

				// If a target_ini is defined copy that to the cloned directory
				if (!target_ini.empty()) {
					fs::path ini = fs::absolute(fs::path(_config.File()).parent_path() / target_ini);
					if (_verbose) std::cout << "Copying '" + target_ini + "' to '" + dir.string() + "'\n";
					fs::copy_file(ini, dir / "bosc.ini", fs::copy_options::overwrite_existing);
				}
			}
			else {
				if (_verbose) std::cout << "Directory '" << target << " already exists, skipping.\n";
			}

		} else if (type == "path") { // import is a path
					     // just convert the path to the canonical form
			dir = fs::canonical(target);
			if (_verbose) {
				if (_namespace != _name) {
					std::cout << "\n[" << _name << "]" << "\n";
					_namespace = _name;
				}
				std::cout << "Using path " << dir << " for " << key << "\n";
			}
		} else throw std::runtime_error("Unknown type '" + type + "' for import '" + key + "'");

		// Add the project to the list with the download path
		_projects.Add(key, "path", dir.string());

	}

	// Once all dependencies have been downloaded, handle subdependencies
	_projects.Save(_projects.File()); // Save the changes to the file

	for (const auto& sec: _config.Keys("import")) {

		_child.emplace_back(_projects.GetString(sec, "path", ""), _verbose);
		_child.back().import();
	}

	// Once all is done, add it's own entry
	if(_projects.HasValue(_name, "path")) return;

	_projects.Add(_name, "path", fs::path(_config.File()).parent_path().string());
	_projects.Save(_projects.File());

}

bool Bosc::run_hook(const std::string& stage) {
	if (_config.HasValue(stage, "hook")) {
		if (_namespace != _name) {
			std::cout << "\n[" << _name << "]" << "\n";
			_namespace = _name;
		}
		std::cout << " - Running pre-" << stage <<" hooks\n";
	}
	for (const auto& hook : _config.GetStringList(stage, "hook", {})) {
		std::string quiet = _verbose ? "" : " > /dev/null 2>&1";
		if (_verbose) std::cout << hook << "\n";
		int res = std::system((hook + quiet).c_str());
		if (res != 0) throw std::runtime_error("Pre-" + stage + " hook '" + hook + "' failed");
	}
	return true;
}

bool Bosc::build() {
	bool dirty = false;

	// Build dependencies
	for (auto& c: _child) dirty |= c.build();
	
	// Create build dir if it doesn't exist
	fs::path pdir = fs::path(_config.File()).parent_path();
	fs::path bdir = pdir / ("build-" + _hash);
	fs::create_directories(bdir);
	const fs::path target = bdir / _config.GetString("build", "target", _name);

	dirty |= !fs::exists(target); // If not bulid is also dirty

	// Check if build is dirty (a source is more recent than the target)
	for (auto& f : _config.GetStringList("build", "src", {})) {
		fs::path file = fs::path(f);
		file = pdir / file;
		if (valid_obj(target, file)) continue;
		dirty = true;
		break;
	}

	if (dirty && _config.HasValue("build", "hook")) {
		if (_namespace != _name) {
			std::cout << "\n[" << _name << "]" << "\n";
			_namespace = _name;
		}
		std::cout << " - Running pre-build hooks\n";
		for (const auto& hook : _config.GetStringList("build", "hook", {})) {
			std::string quiet = _verbose ? "" : " > /dev/null 2>&1";
			if (_verbose) std::cout << hook << "\n";
			int res = std::system((hook + quiet).c_str());
			if (res != 0) throw std::runtime_error("Pre-build hook '" + hook + "' failed");
		}
	}


	const std::string gcc = std::vformat(_compiler, std::make_format_args("gcc"));
	const std::string gpp = std::vformat(_compiler, std::make_format_args("g++"));
	const std::string ar = [&] {
		auto pos = _compiler.find("{}");
		std::string fmt = (pos == std::string::npos) ? _compiler : _compiler.substr(0, pos) + "{}";
		return std::vformat(fmt, std::make_format_args("ar"));
	}();
	const std::string flags = list_to_string(_config.GetStringList("build", "flags", {}), "");
	const std::string cflags = list_to_string(_config.GetStringList("build", "cflags", {}), "");
	const std::string cppflags = list_to_string(_config.GetStringList("build", "cppflags", {}), "");
	std::string incl = list_to_string(_config.GetStringList("build", "include", {}), std::string("-I") + pdir.string() + fs::path::preferred_separator);

	std::string imports;
	for (auto& c : _child) {
		imports += c._exports;
		_libs += c._libs;
	}

	incl += imports;

	std::string objs;
	std::string cmd;

	static const std::unordered_set<std::string> exts{".c", ".s", ".S"};
	for (auto& f : _config.GetStringList("build", "src", {})) {
		fs::path file = fs::path(f);
		const std::string compiler = (exts.count(file.extension().string()) > 0) ? gcc : gpp;
		const std::string fflags = flags + ((exts.count(file.extension().string()) > 0) ? cflags : cppflags);
		fs::path out = bdir / file;
		fs::create_directories(out.parent_path());
		out.replace_extension(".o");
		objs += out.string() + " ";
		file = pdir / file;
		if (valid_obj(out, file)) continue;
		if (_namespace != _name) {
			std::cout << "\n[" << _name << "]" << "\n";
			_namespace = _name;
		}
		std::cout << " - Building " << file.stem().string() << "\n";
		dirty = true;
		cmd = compiler + "-c " + file.string() + " -o " + out.string() + " " + fflags + " " + incl;
		if (_verbose) std::cout << cmd << "\n";
		int res = std::system(cmd.c_str());
		if (res != 0) throw std::runtime_error("Build of " + out.string() + " failed");
	}

	const std::string ldflags = list_to_string(_config.GetStringList("build", "ldflags", {}), "-Wl,");

	fs::create_directories(target.parent_path());

	std::string msg;
	if (target.extension() == ".a") {
		msg = " - Packing " + target.stem().string();
		cmd = ar + " rcs " + target.string() + " " + objs;
		_libs += target.string() + " ";
	}
	else {
		msg = " - Linking " + target.stem().string();
		cmd = gpp + " " + objs + " -o " + target.string() + " " + ldflags + _libs;
	}

	if (dirty) {
		if (_namespace != _name) {
			std::cout << "\n[" << _name << "]" << "\n";
			_namespace = _name;
		}
		std::cout << msg << "\n";
		if (_verbose) std::cout << cmd << "\n";
		int res = std::system(cmd.c_str());
		if (res != 0) throw std::runtime_error("Generation of " + target.string() + " failed");
	}

	// Export
	_exports += imports + list_to_string(_config.GetStringList("export", "include", {}), std::string("-I") + pdir.string() + fs::path::preferred_separator);

	return dirty;
}

void Bosc::install() {
	fs::path bdir = fs::path(_config.File()).parent_path() / ("build-" + _hash);
	fs::path target = fs::path(_config.GetString("build", "target", _name));
#ifdef _WIN32
	if (!target.has_extension()) target += ".exe";
#endif
	fs::path src = bdir / target;
	fs::path idir = (fs::path(_config.GetString("install", "path", "")) / _name) / target.parent_path();
	if (_namespace != _name) {
		std::cout << "\n[" << _name << "]" << "\n";
		_namespace = _name;
	}
	std::cout << " - Installing " << _name << "\n";
	if (_verbose) std::cout << "Copying '" << src.string() << "' to '" << idir.string() << "'\n";
	if (access(idir.c_str(), W_OK) != 0) {
		std::string cmd = std::string("sudo mkdir -p ") + idir.string();
		if (_verbose) std::cout << cmd << "\n";
		int res = std::system(cmd.c_str());
		if (res != 0) throw std::runtime_error("Creation of " + idir.string() + " failed");
		cmd = std::string("sudo cp ") + src.string() + " " + idir.string();
		if (_verbose) std::cout << cmd << "\n";
		res = std::system(cmd.c_str());
		if (res != 0) throw std::runtime_error("Copy of " + src.string() + " to " + idir.string() + " failed");
	} else {
		fs::create_directories(idir);
		fs::copy(src, idir, fs::copy_options::overwrite_existing);
	}

}

void Bosc::uninstall() {
	fs::path idir = fs::path(_config.GetString("install", "path", "")) / _name;
	if (fs::exists(idir) && _config.HasValue("install", "path")) {
		if (_namespace != _name) {
			std::cout << "\n[" << _name << "]" << "\n";
			_namespace = _name;
		}
		std::cout << " - Uninstalling " << _name << "\n";


		if (_verbose) std::cout << "Removing '" << idir.string() << "'\n";
		if (access(idir.c_str(), W_OK) != 0) {
			std::string cmd = std::string("sudo rm -r ") + idir.string();
			if (_verbose) std::cout << cmd << "\n";
			int res = std::system(cmd.c_str());
			if (res != 0) throw std::runtime_error("Failed to remove " + idir.string());
		} else remove_path(idir);
	} else if (_verbose) {
		if (_namespace != _name) {
			std::cout << "\n[" << _name << "]" << "\n";
			_namespace = _name;
		}
		std::cout << "Nothing to uninstall\n";
	}
}

void Bosc::clean(bool recursive) {
	if (recursive) for (auto& c: _child) c.clean(recursive);

	fs::path pdir = fs::path(_config.File()).parent_path();
	fs::path bdir = pdir / ("build-" + _hash);

	if (!fs::exists(bdir)) {
		if (_verbose)  {
			if (_namespace != _name) {
				std::cout << "\n[" << _name << "]" << "\n";
				_namespace = _name;
			}
			std::cout << "Build already cleaned\n";
		}
		return;
	}

	if (_namespace != _name) {
		std::cout << "\n[" << _name << "]" << "\n";
		_namespace = _name;
	}
	std::cout << " - Cleaning build '" << _hash <<"'\n";

	if (_verbose) std::cout << "Removing '" << bdir.string() <<"'\n";
	remove_path(bdir);
}

void Bosc::purge(bool recursive) {
	if (recursive) for (auto& c: _child) c.purge(recursive);

	fs::path pdir = fs::path(_config.File()).parent_path();

	std::vector<std::string> hashes;
	std::regex pattern(R"(build-([0-9a-fA-F]{8}))");
	std::smatch match;

	for (const auto& entry : fs::directory_iterator(pdir)) {
		if (entry.is_directory()) {
			const auto name = entry.path().filename().string();
			if (std::regex_match(name, match, pattern)) {
				hashes.push_back(match[1]);
			}
		}
	}

	for (const auto& hash : hashes) {
		fs::path bdir = pdir / ("build-" + hash);
		if (_namespace != _name) {
			std::cout << "\n[" << _name << "]" << "\n";
			_namespace = _name;
		}
		std::cout << " - Cleaning build '" << hash <<"'\n";

		if (_verbose) std::cout << "Removing '" << bdir.string() <<"'\n";
		remove_path(bdir);
	}

}

void Bosc::remove(bool recursive) {
	if (recursive) for (auto& c: _child) c.remove(recursive);
	uninstall();
	purge(false);

	fs::path pdir = fs::path(_config.File()).parent_path();

	_projects.Remove(_name, "path");
	_projects.Save(_projects.File());

	if (is_subdir(fs::path(_projects.File()).parent_path(), pdir)) {
		if (_namespace != _name) {
			std::cout << "\n[" << _name << "]" << "\n";
			_namespace = _name;
		}
		std::cout << " - Removing " << _name << " git directory\n";
		if(_verbose) std::cout << "Deleting '" << pdir.string() <<"'\n";
		remove_path(pdir);
	}

}
