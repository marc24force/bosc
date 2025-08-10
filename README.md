# BOSC - Build Orginization and Script in Cascade

Bosc is a build system for C/C++ projects that manages dependencies, builds, installs, and cleans projects using a `bosc.ini` configuration file.

It uses [Ciri](https://github.com/marc24force/ciri.git) for parsing the ini files.
See `Configuration` for the characteristics of using Ciri.

## Features

* Manage project dependencies (git repos or local paths)
* Manage different project builds depending on the compiler and flags used
* Supports command-line flags for verbosity, recursion, and project selection
* Use arguments and cross-references inside `bosc.ini`

## Usage

```
bosc [flags] <cmd> [args]
```

### Commands

* `import`: Download dependencies
* `build`: Build project and dependencies
* `install`: Install project
* `uninstall`: Uninstall project
* `clean`: Clean current build
* `purge`: Clean all builds
* `remove`: Remove project entry and delete repository if applicable

### Flags

* `-v`, `--verbose`: Enable verbose output
* `-r`, `--recursive`: Use with `clean`, `purge`, `remove` for recursive action
* `-p project`, `--project=project`: Specify project for `clean`, `purge`, `remove`
* `-h`, `--help`: Show help message

## Configuration - bosc.ini

The `bosc.ini` file controls project settings and must be placed in the root project directory.

### Sections and Keys

* **\[import]**

  Define dependencies:
  * `dep = {repo, git-url [, version [, config.ini]]}`
  * `dep = {path, local-path}`

* **\[compiler]**
  * `path` (string): compiler executable
  * `prefix` (string): compiler prefix
  * `flags` (list): flags affecting build hash

* **\[build]**
  * `hook` (list): list of commands to run before building
  * `flags` (list): generic build flags
  * `cflags` (list): C-specific flags
  * `cppflags` (list): C++-specific flags
  * `ldflags` (list): linker flags
  * `src` (list): source files
  * `include` (list): include directories
  * `target` (string): output binary or library

* **\[install]**
  * `path` (string): installation directory
  *Note: a subdirectory named after the project (taken from the root folder name) will be created inside this path*

* **\[export]**
  * `include` (list): headers and include paths for dependent projects

### Features

* Command-line arguments accessible via `$(1)`, `$(2)`, etc. inside values
* Cross-references within the file using `${section=value}` syntax

### File Format

* Sections: [name]
* Key-value pairs: `key = value`
* Values can be strings or lists (`{item1, item2, ...}`)
* Comments:

  * Lines starting with `#` are full-line comments
  * Semicolons `;` start comments after an entry on the same line

## Building

The provided `Makefile` builds Bosc with double redundancy: it compiles the tool, then uses it to compile itself once more to ensure reproducible builds.

## Example bosc.ini

```ini
[import]
projectA = {repo, git@git.com:user/projectA.git}
projectB = {path, ../localdep/projectB}

[compiler]
path = /usr/bin/gcc
prefix = riscv64-unknown-elf-
flags = {-march=rv64imafd, -mabi=lp64d}

[build]
flags = {-DENABLE_FEATURE}
cflags = {-std=c17}
cppflags = {-std=c++17}
ldflags = {-Tlink.ld, -static}
src = {src/main.c, utils/src/Class.cpp}
include = {include, utils/include}
target = bin/project.exe

[install]
path = /opt

[export]
include = {include/, extra/include}

