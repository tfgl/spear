# Commands
 - [x] spear new <project_name>
 - [x] spear run [debug/release] (debug is default)
 - [x] spear build [debug/release] (debug is default)
 - [x] clean
 - [x] install [debug/release] (debug is default)
 - [ ] package
 - [ ] fetch
 - [x] help
 - [ ] test (run all the test in test/)
 - [o] add <lib> [<version>] [--local]
        --local tell spear not to download anything, use the localy provided libs. cmd_args will still be looked up from the github vault if not provided localy
 - [ ] remove <lib> <version>

# Spear config
 - [x] add toml config file in $XDG_CONFIG_HOME/spear.toml or $HOME/.config/spear.toml
 - file:
  ```toml
  user = <name> #use to fill author field
  cc = <compiler>
  ```

# Project config
 - file:
  ```toml
  [project]
  name = <name>
  version = <version>
  cc = <compiler>
  authors = [<author>, ...]
 
  [dependencies.name]
  version = <x.x.x> #version of the library
  command = [<arg>, ...]

  #not implemented
  [compiler]
  name = <name> #name = clangd
  options = [<op>, ...] # options = -Og -g3
  ```
  - TODO:
   - [ ] add a compiler section with:
     - [x] a variable to overwrite the default compiler
     - [ ] a variable to overwrite the default compiler command options (except the -o options)

# Local storage
 - $XDG_DATA_HOME/spear/libs.toml:
 list of all the libraries and their versions known to spear.
 Tell spear where to get specific library.
 Which versions it already fetched.
 How to include them in the build process.
 
 ```toml
 [lib_name]
 url = <url> #the library repository
 commands = [<arg>, ...] #the arguments to add to compile with the library (ex: -lsdl2)
 fetched = [<x.x.x>, ...] #all the fetched versions of the library
 versions = [<x.x.x> = <branch>, ...] #all the published versions of the library
 ```
 - $XDG_DATA_HOME/spear/libs/:
 store all the fetched libraries
 ```
 libs.
     ├──lib_name_1
     │  ├── ...
     │  ├── ...
     │  ├── ...
     .  .
     .  .
     .  .
     ├──lib_name_2
     │  ├── ...
     │  ├── ...
     │  ├── ...
     .  .
     .  .
     .  .
 ```
