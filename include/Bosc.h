#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "Bruc.h"


namespace fs {
	using path = std::filesystem::path;
	using list = std::vector<path>;
}

//namespace fs = std::filesystem;

class Bosc {
	private:
		// Local variables
		Bruc b;
		fs::path dir_local;
		bool is_root;
		std::string depend_flags_local;
		std::string depend_flags_parent;

		// Structs
		struct Compiler {
			fs::path path; 
			std::string prefix;
			std::string flags;
		};

		struct Export {
			std::string flags;
			fs::list includes;
			fs::list lpaths;
			std::vector<std::string> lnames;
		};

		// Static variables
		static fs::path _repos;
		static Compiler _compiler;
		static Export _export;

		// Utility functions
		int load_bruc();
		fs::path make_absolute(fs::path p);
		int create_dir(fs::path dir);
		int get_dep_path(fs::path& path, std::string name);
		int build_dependency(std::string name);
		bool is_older(fs::path p1, fs::path p2);


	public:
		Bosc(fs::path dir, Bosc* parent);
		int build(bool install);
		int clean(bool recursive);
		int uninstall();
		int purge();

};
