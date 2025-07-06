# BOSC - Build Orginization and Script in Cascade

## TODO
- First download deps, then build
- Install path, require dir name (not use project)
- Default install path "."
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

[compiler] # section that defines the compiler used
           # only executed in the root project and shared with all dependencies

path = /path/to/compiler     # path to the compler location if not in PATH
prefix = riscv64-unknown-elf # prefix of the compiler if not gcc or g++
flags = {-march=rv64imafd, -mabi=lp64d, -mcmodel=medany} # list of compiler flags

[import] # section that defines dependencies
         # executed in first place when building
bruc = {repo, git...}
bruc2 = {path, ../..}

flags = {-DDEBUG=1, -DNPRINT} # List of flags to pass to all dependencies
dir = .repos   # Path to directory where to download dependencies
script = /path/to/script # Path to script to execute before downloading dependencies

[build]  # section that defines the build
         # executed both as main and as dependency

dir = build # Directory where the build files are stored
flags = {-DTYPE=float} # Flags to use when bulding
sources = {src/main.c, utils/src/Class.cpp, extra/tools.S} # List of files to compile
script = /path/to/script # Path to script to execute before building

[link]

dir = bin # Directory where the output is stored
flags = {-Tlink.ld, -static} # Flags used when linking the project
target = project.exe # Output of the project
script = /path/to/script # Path to script to execute before linking

[export] # section that defines how to use this as dependency
         # executed last when building

includes = {includes} # list of includes
flags = {flag1, flag2, flag3} # list of flags

[install] # section that defines install information
          # only executed by the root if the section exists

path = "/opt/project" # Path were to install the project
script = /path/to/script # Path to script to execute before linking


```

```bash
bosc build ARGS     # builds the project
bosc install ARGS   # installs the project
bosc clean ARGS     # cleans the project
bosc clean-all ARGS # cleans the project and dependencies
bosc purge ARGS     # clean-all + removes git repositories downloaded
```
