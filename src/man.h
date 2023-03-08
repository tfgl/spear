#pragma once
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
"          <lib>     is the library name you want to add\n"
"          <version> Optinal. version of the library, latest by default\n"
"          --local   if added, tell spear the library is already provided by the system\n";

static std::string build =
"spear bulid [debug/release]\n";
}
