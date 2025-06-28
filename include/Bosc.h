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
	public:

		Bruc conf;

		static fs::path dir_repos;
		fs::path dir_local;

		// compiler section //global
		static fs::path compiler_path; 
		static std::string compiler_prefix;
		static std::string compiler_flags;

		// depend section
		std::string depend_flags_local;
		std::string depend_flags_parent;

		// export section //global
		static std::string export_flags;
		static fs::list export_includes;
		static fs::list export_libs_path;
		static std::vector<std::string> export_libs_name;

		// project section
		std::string project_link;

		fs::path getAbsPath(fs::path p);

		void init();
		void depend();
		void build();
		//void install();
		void clean();
		void clean_deps();
		//void purge();

		Bosc(fs::path local_dir, Bosc* parent);
		~Bosc();
};
