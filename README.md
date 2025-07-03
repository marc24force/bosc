# BOSC - Build Orginization and Script in Cascade

## TODO
- arguments
- clean code
- Automatic linker (ld if pure c, g++ if c++)
- Append -Wl to linker flags if c++
- scripts depend, build and install

## Install

Set the install directory by modifying the bosc.bruc file. 
Depending on the target directory the install command will require sudo.

```bash
make
./make_bosc.exe install
```

## Example
```ini

# This section provides information on the compiler used to build the project
[compiler] 

path = "/path/to/compiler/bin"
prefix = "riscv64-unknown-elf-"
flags = "-march=rv64imafd -mabi=lp64d -mcmodel=medany"

# This section descrives the required dependencies to build the project
[depend]

project1 = {path, /path/to/project1}
project2 = {repo, https://git.com/user/repo} # TODO, master}
project3 = {repo, git@git.com:user/rep} # TODO, dev}

flags = "-DDEBUG=1" # Flags to be used when building dependencies

# This section provides information for projects that have this as dependency

[export] 

flags = "--std=c++11" # Flags that are required for projects using this project
includes = {/path/to/project/export/includes} # include directories to export

[project]

name = "MyProject"

targets = {bin/project.exe, lib/libproject.a}

flags = "-DTYPE=float" # Flags used for building this project
includes = {include/, utils/include}
sources = {src/main.c, utils/src/Class.cpp, extra/tools.S} # Wildcard not supported
link = "-Tlink.ld -Wl,static" # Flags to be used when linking (linking is done usign g++)

[dirs]
build = "build"
install = "/opt"
repos = ".bosc/repos" # Path were to download the dependencies


```

```bash
bosc build ARGS     # builds the project
bosc install ARGS   # installs the project
bosc clean ARGS     # cleans the project
bosc clean-all ARGS # cleans the project and dependencies
bosc purge ARGS     # clean-all + removes git repositories downloaded
```
