#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "Bruc.h"


namespace fs {
	using path = std::filesystem::path;
	using list = std::vector<path>;
}


class Bosc {
	private:
		// Structs
		struct Compiler {
			fs::path path; 
                        fs::path prefix;
                        std::string flags;
		};

		struct Import {
                	fs::list child;
                        fs::path dir;
                        std::string flags;
                        fs::path script;
                };

                struct Build {
                	fs::path dir;
                        std::string flags;
                        fs::list srcs;
                        fs::path script;
                };

                struct Link {
                	fs::path dir;
                        std::string flags;
                        fs::path target;
                        fs::path script;
                }

		struct Export {
			fs::list incs;
			std::string flags;
		};

		struct Install {
			fs::path path;
                        fs::path script;
                };
			
		// Local variables
		Compiler _compiler;
		Import _import;
		Build _build;
		Link _link;
		Export _export;
		Install _install;

		Bruc b;
		Bosc* parent;


		// Utility functions
		int load_bruc();
		fs::path make_absolute(fs::path p);
		int create_dir(fs::path dir);
		int get_dep_path(fs::path& path, std::string name);
		int download_all();
		int build_dependency(std::string name);
		bool is_older(fs::path p1, fs::path p2);


	public:
		Bosc(fs::path dir, Bosc* parent);
		int build(bool install);
		int clean(bool recursive);
		int uninstall();
		int purge();

};
