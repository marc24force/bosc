#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include "Ciri.h"

// Build manager class
class Bosc {
	public:

		// Bosc: Initializes the build manager.
		// Args: dir = build directory, verbose = enables detailed logging.
		// Fail: Throws if parsing dir/bosc.ini fails.
		Bosc(const std::string& dir, bool verbose);

		// import: Downloads all project dependencies if not existing.
		// Updates the list of projects in $HOME/.local/share/bosc.
		// Fail: Throws if it can't download a dependency or access the project list.
		void import();
		
		// build: Builds all project dependencies before building itself.
		// Return: True if build updated, false if nothing done.
		// Fail: Throws if an error occurs during build.
		bool build();

		// install: Installs the current build.
		// Fail: Throws if an error occurs when installing.
		void install();

		// uninstall: Removes the current build installation.
		// Fail: Throws if an error occurs when uninstalling.
		void uninstall();
		
		// clean: Removes the current build files.
		// Args: Indicates if recursively clean dependencies.
		// Fail: Throws if an error occurs when cleaning.
		void clean(bool recursive);
		
		// purge: Removes the all build files.
		// Args: Indicates if recursively purge dependencies.
		// Fail: Throws if an error occurs when purging.
		void purge(bool recursive);
		
		// remove: Removes the project entry and if it's a repository deletes it.
		// Args: Indicates if recursively remove dependencies.
		// Fail: Throws if an error occurs when removing.
		void remove(bool recursive);

		/* Static functions */
		static Ciri& projects();

	private:
		/* Static variables */
		
		// Compiler data
		static std::string _compiler;
	
		// Build hash depending on compiler and flags
		static std::string _hash;

		// Project information file
		static Ciri _projects;

		// It's the root project, only the root will read as true
		static bool _root;

		// Used for printing the current project info
		static std::string _namespace;

		/* Constant variables */
		
		// Project name
		const std::string _name;

		// Enables detailed logging.
		const bool _verbose;

		/* Local variables */
		
		// Project configuration file
		Ciri _config;

		// List of child projects
		std::vector<Bosc> _child;

		// Export directories for include
		std::string _exports;

		// Libs used + own
		std::string _libs;


		/* Helper functions */

		// run_hook: Executes the pre-stage hook.
		// Args: String indicating the stage to hook.
		// Return: True on success
		// Fail: Throws if the hooks terminates with non 0 return.
		bool run_hook(const std::string& stage);
};

