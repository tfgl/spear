#include "spear.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sched.h>
#include <string>
#include <filesystem>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "toml.h"
#include "man.h"

namespace fs = std::filesystem;

char cc[] = "g++";
std::string xdg_config_home(getenv("XDG_CONFIG_HOME"));
std::string xdg_data_home(getenv("XDG_DATA_HOME"));

std::string glb_config_path = xdg_config_home.compare("") == 0
  ? std::string(getenv("HOME")) + "/.config/spear.toml"
  : xdg_config_home + "/spear.toml";

// TODO create dir & file when installing
std::string libs_config_path = xdg_data_home.compare("") == 0
  ? std::string(getenv("HOME")) + "/.local/share/spear/libs.toml"
  : xdg_data_home + "/spear/libs.toml";

auto glb_config = toml::parse_file(glb_config_path);

std::string hello_world_prgm = 
R"(#include <iostream>

int main(int argc, char* argv[]) {
  std::cout<< "hello, world" << std::endl;
  return 0;
})";

std::vector<char*> get_dependencies() {
  std::vector<char*> deps_arg;

  auto config = toml::parse_file("../spear.toml");
  auto dependencies = config.at_path("dependencies");
  dependencies.as_table()->for_each([&deps_arg](toml::table& dependency) {
    if( auto arr = dependency.at_path("command").as_array() ) {
      arr->for_each([&deps_arg](toml::value<std::string>& arg) {
        deps_arg.push_back(arg->data());
      });
    }
  });

  return deps_arg;
}

void new_project(const int argc, char* argv[]) {
  CHECK((argc < 1), man::new_)
  std::string project_name(argv[1]);
  if (fs::exists(project_name)) {
    std::cout << "project name already taken" << std::endl;
    return;
  }

  std::string directories[] = {
    project_name,
    std::string(project_name) + "/src",
  };

  for (auto dir: directories)
    fs::create_directory(dir);

  std::ofstream f_main(project_name + "/src/main.cpp");
  f_main << hello_world_prgm << std::endl;
  f_main.close();

  std::ofstream f_spear(project_name + "/spear.toml");
  f_spear << "[project]\n"
          << "name = '" << argv[1] << "'\n"
          << "version = '0.1.0'\n"
          << "authors = ["<< glb_config.at_path("user") <<"]\n"
          << "\n"
          << "[dependencies]\n";

  f_spear.close();

  pid_t pid = fork();
  if (pid == 0) {
    char* args[] = {"git", "init", project_name.data(), NULL};
    execvp(args[0], args);
  } else {
    wait(NULL);
    auto of = std::ofstream(project_name + "/.gitignore");
    of << "target/"
       << std::endl;
  }
}

std::vector<char*> build_objects(std::string base_output_dir, std::vector<char*>& cmd_args) {
  fs::current_path("src/");
  fs::path output_dir("../"+base_output_dir);
  std::vector<char*> args = {cc};
  auto dependencies = get_dependencies();
  args.insert(args.end(), dependencies.begin(), dependencies.end());

  std::cout << "BUILDING" << std::endl;
  for (auto& file: fs::recursive_directory_iterator{"."}) {
    if(file.is_directory()) {
      fs::create_directory(output_dir.append(file.path().stem().string()));
      continue;
    }

    std::string white_list[] = {".c", ".cpp", "c++", "cxx"};
    if (std::find(std::begin(white_list), std::end(white_list),
                  file.path().extension().string().c_str()) == std::end(white_list))
      continue;

    std::string object_file = output_dir.string().append(file.path().parent_path().append(file.path().stem().string().append(".o")));
    args.push_back(strdup(object_file.c_str()));

    auto object_path = fs::path{object_file};
    if( fs::exists(object_path) && fs::last_write_time(object_path) > fs::last_write_time(file) )
      continue;

    std::cout << "file: " << file.path().string() << std::endl;

    pid_t pid = fork();
    if (pid == 0) {
      cmd_args.push_back(object_file.data());
      cmd_args.push_back(file.path().string().data());
      cmd_args.push_back(NULL);

      execvp(cmd_args[0], cmd_args.data());
      exit(0);
    } else {
      wait(NULL);
    }
  }

  args.push_back("-o");
  return args;
}

void build(const int argc, char* argv[]) {
  std::string project_name = toml::parse_file("spear.toml").at_path("project.name").value_or("");
  std::string target_dir;
  std::string target;
  std::vector<char*> build_args;

  fs::create_directory("target/");

  if (argc >= 2 && std::string(argv[1]).compare("release") == 0) {
    target_dir = "target/release/";
    build_args = {cc, "-c", "-O3", "-std=c++20", "-o"};
  }
  else {
    target_dir = "target/debug/";
    build_args = {cc, "-c", "-Og", "-g3", "-Wall", "-std=c++20", "-o"};
  }

  fs::create_directory(target_dir);
  target = target_dir + project_name.c_str();

  auto args = build_objects(target_dir, build_args);
  args.push_back(strdup(("../" + target).c_str()));

  std::cout << "LINKING" << std::endl;
  for(auto& arg: args)
    std::cout << arg << " ";
  std::cout << std::endl;

  args.push_back(NULL);
  execvp(args[0], args.data());
}

void run(const int argc, char* argv[]) {
  pid_t pid = fork();
  if (pid == 0) {
    build(argc, argv);
    exit(0);
  }
  else {
    wait(NULL);
  }

  std::vector<char*> command;

  std::string project_name = toml::parse_file("spear.toml").at_path("project.name").value_or("");
  std::string target = (argc >= 2 && std::string("release").compare(argv[1]) == 0) ? "target/release/" + project_name : "target/debug/" + project_name;
  command.push_back(target.data());

  int with_position = 0;
  if( argc >= 2 && std::string("with").compare(argv[1]) == 0 ) // spear run with a b c d
    with_position = 2;
  if( argc >= 3 && std::string("with").compare(argv[2]) == 0 ) // spear run release/debug with a b c d
    with_position = 3;

  if( with_position != 0 ) {
    for(int i=with_position; i<argc; i++) {
      command.push_back(argv[i]);
    }
  }

  command.push_back(NULL);
  std::cout << "RUNING" << std::endl;
  execvp(command[0], command.data());
}

void clean() {
  fs::remove_all("target");
}

void package(const int argc, char* argv[]) {
  // TODO
}

void fetch(const int argc, char* argv[]) {
  // TODO
}

void add(const int argc, char* argv[]) {
  CHECK((argc < 2), man::add)

  std::string lib_name;
  std::string lib_version;
  bool is_localy_provided = false;

  int max = argc > 3 ? 3 : argc;
  for (int i=1; i<max; i++) {
    if (std::string("--local").compare(argv[i]) == 0) {
      is_localy_provided = true;
    }
    else if (lib_name.empty()) {
      lib_name = argv[i];
    }
    else {
      lib_version = argv[i];
    }
  }

  auto libs_cfg = toml::parse_file(libs_config_path);
  auto lib_cfg = libs_cfg[lib_name];
  auto commands = lib_cfg["command"];

  auto config = toml::parse_file("spear.toml");
  std::string cfg_string =
                           "version = '"+lib_version+"'\n"
                           "command = [";

  commands.as_array()->for_each([&cfg_string](toml::value<std::string> el) {
    cfg_string.append("'" + std::string(el->data()) + "'," );
  });
  cfg_string.back() = ']';

  auto cfg = toml::parse(cfg_string);
  
  config.emplace("dependencies", toml::table{});
  auto tbl = config["dependencies"].as_table();
  tbl->insert_or_assign(lib_name, cfg);

  std::ofstream("spear.toml") << config;
  if(argc > 4)
    add(argc - 2, argv+2);
}

void spear(const int argc, char* argv[]) {
  CHECK((argc < 2 || std::string("help").compare(argv[1]) == 0), man::spear)
  std::string argv1(argv[1]);

  if (argv1.compare("new") == 0) {
    new_project(argc - 1, argv + 1);
  } else if (argv1.compare("run") == 0) {
    run(argc - 1, argv+1);
  } else if (argv1.compare("build") == 0) {
    build(argc - 1, argv+1);
  } else if (argv1.compare("clean") == 0) {
    clean();
  } else if (argv1.compare("package") == 0) {
    package(argc - 1, argv+1);
  } else if (argv1.compare("fetch") == 0) {
    fetch(argc - 1, argv+1);
  } else if (argv1.compare("add") == 0) {
    add(argc - 1, argv+1);
  } else if (argv1.compare("get_name") == 0) {
    std::cout << toml::parse_file("spear.toml").at_path("project.name") << std::endl;
  } else {
    std::cout << man::spear << std::endl;
  }
}
