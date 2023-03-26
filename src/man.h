#pragma once
#include "spear.h"
#include <iostream>

#define CHECK(value, help)\
if (value) {\
  std::cout << help << std::endl;\
  return;\
}

namespace man {
static std::string spear =
"spear | new   <name>          | create a new project\n"
"      | build [debug/release] | build the current project\n"
"      | run   [debug/release] | run the current project\n"
"      | clean                 | clean the project build\n"
"      | package               | package the project into a library\n"
"      | fetch                 | fetch the dependencies from spear.toml\n";

static std::string new_ =
"spear new <name>"
"create a new project";

static std::string add =
"spear add <lib> <version>\n"
"          <lib>           -- is the library name you want to add\n"
"          <version>       -- Optinal. version of the library, latest by default\n"
"          --local         -- if added, tell spear the library is already provided by the system\n";

static std::string build =
"spear bulid [debug/release]\n";

static std::string enable_feature =
"spear enable <lib_name> <features>\n"
"             <lib_name>            -- is the name of the library providing the features\n"
"             <features>            -- is the features you want to enable (you can specify multiple features at once)\n"
"example:\n"
"spear enable sdl2 image ttf        -- enable the image and ttf features of the sdl2 library";

}
