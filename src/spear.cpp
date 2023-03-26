#include "spear.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
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
#include <vector>

#include "toml.h"
#include "man.h"

namespace fs = std::filesystem;
using std::string;
using std::vector;
using strvec = vector<string>;
using path = fs::path;

string cc = "clang++";

string hello_world_prgm = 
"#include <iostream>\n"
"\n"
"int main(int argc, char* argv[]) {\n"
"    std::cout<< \"hello, world\" << std::endl;\n"
"    return 0;\n"
"};\n";

path root = fs::current_path();

toml::Table lib_configs;
toml::Table project_config;
toml::Table global_config;

void find_project_config() {
    project_config = toml::parse(root / "spear.toml");
}

void find_global_config() {
    static path xdg_config_home(getenv("XDG_CONFIG_HOME"));
    static path home(getenv("HOME"));

    static path global_config_path = fs::exists(xdg_config_home)
        ? xdg_config_home / "spear.toml"
        : home / ".config"/"spear.toml";

    global_config = toml::parse(global_config_path);
}

void find_lib_configs() {
    path xdg_data_home(getenv("XDG_DATA_HOME"));
    path home(getenv("HOME"));

    // TODO create dir & file when installing
    path libs_config_path = fs::exists(xdg_data_home)
        ? xdg_data_home / "spear"/"libs.toml"
        : home / ".local"/"share"/"spear"/"libs.toml";

    auto file = toml::parse(libs_config_path);

    lib_configs = file;
}

void find_root() {
    while ( !fs::exists(root / "spear.toml") && root.root_path() != root ) {
        root = root.parent_path();
    }
}

void m_execvp(strvec cmd) {
    char** c_cmd = (char**)malloc((cmd.size() + 1) * sizeof(char*));

    for (size_t i=0; i<cmd.size(); i++) {
        c_cmd[i] = (char*)malloc(cmd[i].size() * sizeof(char) + 1);
        strcpy(c_cmd[i], cmd[i].c_str());
        std::cout << cmd[i] << " ";
    }
    std::cout << std::endl;
    c_cmd[cmd.size()] = NULL;

    execvp(c_cmd[0], c_cmd);
}

strvec get_dependencies() {
    strvec deps_arg;
    std::cout << "GET_DEPENDENCIES" << std::endl;

    auto dependencies = project_config["dependencies"]->as<toml::Table>();

    dependencies->foreach([&deps_arg](toml::Node* raw_dependency) {
        std::cout << "deps found" << std::endl;
        auto dependency = raw_dependency->as<toml::Table>();
        if( auto arr = (*dependency)["command"]->as<toml::Array>() ) {
            arr->foreach([&deps_arg](toml::Node* arg) {
                deps_arg.push_back(((toml::String*)arg)->_data);
            });
        }
    });

    std::cout << "DONE" << std::endl;

    return deps_arg;
}

void new_project(const int argc, char* argv[]) {
    CHECK((argc < 1), man::new_)
    fs::path project_name(argv[1]);

    if (fs::exists(project_name)) {
        std::cout << "project name already taken" << std::endl;
        return;
    }

    // create tree structure
    fs::create_directory(project_name);
    fs::create_directory(project_name / "src");

    // write default hello world
    std::ofstream f_main(project_name / "src" / "main.cpp");
    f_main << hello_world_prgm << std::endl;
    f_main.close();

    // write default spear toml config
    std::ofstream f_spear(project_name / "spear.toml");
    f_spear << "[project]\n"
        << "name = '" << argv[1] << "'\n"
        << "version = '0.1.0'\n"
        << "authors = ["<< *global_config["user"]->as<toml::String>() <<"]\n";
    f_spear.close();

    // create git repo
    std::cout << "create git repo" << std::endl;
    pid_t pid = fork();
    if (pid == 0) {
        m_execvp({"git", "init", project_name.string()});
        exit(0);
    } else {
        wait(NULL);

        std::ofstream of(project_name / ".gitignore");
        of << "target"
            << std::endl;
        of.close();
    }
}

strvec build_objects(string base_output_dir, strvec& cmd_args) {
    fs::current_path(root / "src");
    path output_dir(root / base_output_dir);
    strvec args = {cc};
    auto dependencies = get_dependencies();
    args.insert(args.end(), dependencies.begin(), dependencies.end());

    std::cout << "BUILDING" << std::endl;
    for (auto& file: fs::recursive_directory_iterator{"."}) {
        if(file.is_directory()) {
            fs::create_directory(output_dir / file.path());
            continue;
        }

        string white_list[] = {".c", ".cpp", "c++", "cxx"};
        if (std::find(std::begin(white_list), std::end(white_list), file.path().extension()) == std::end(white_list))
            continue;

        string object_file = output_dir / file.path().parent_path() / file.path().stem().concat(".o");
        args.push_back(object_file);

        auto object_path = path{object_file};
        if( fs::exists(object_path) && fs::last_write_time(object_path) > fs::last_write_time(file) )
            continue;

        pid_t pid = fork();
        if (pid == 0) {
            cmd_args.push_back(object_file);
            cmd_args.push_back(file.path());

            m_execvp(cmd_args);
            exit(0);
        } else {
            wait(NULL);
        }
    }

    fs::current_path(root);
    args.push_back("-o");
    return args;
}

void build(const int argc, char* argv[]) {
    string project_name = project_config["project.name"]->as<toml::String>()->_data;
    if (project_name == "") project_name = "a.out";

    path target_dir(root / "target");
    path target;
    strvec build_args;

    fs::create_directory(target_dir);

    if (argc >= 2 && string(argv[1]) == "release") {
        target_dir /= "release";
        build_args = {cc, "-I.", "-c", "-O3", "-std=c++20", "-o"};
    }
    else {
        target_dir /= "debug";
        build_args = {cc, "-fdiagnostics-color=always", "-I.", "-c", "-Og", "-g3", "-Wall", "-std=c++20", "-o"};
    }

    fs::create_directory(target_dir);
    fs::create_directory(target_dir / "object");
    fs::create_directory(target_dir / "build");
    target = target_dir / "build" / project_name;

    auto args = build_objects(target_dir / "object", build_args);
    args.push_back(root / target);

    std::cout << "LINKING" << std::endl;

    m_execvp(args);
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

    strvec command;
    string project_name = project_config["project.name"]->as<toml::String>()->_data;

    fs::path target = (argc >= 2 && string(argv[1]) == "release")
        ? root / "target" / "release" / "build" / project_name
        : root / "target" / "debug" / "build" / project_name;
    command.push_back(target);

    int with_position = 0;
    if( argc >= 2 && string(argv[1]) == "with" ) // spear run with a b c d
        with_position = 2;
    if( argc >= 3 && string(argv[2]) == "with" ) // spear run release/debug with a b c d
        with_position = 3;

    if( with_position != 0 ) {
        for(int i=with_position; i<argc; i++) {
            command.push_back(argv[i]);
        }
    }

    std::cout << "RUNING" << std::endl;
    m_execvp(command);
}

void clean() {
    fs::remove_all(root / "target");
}

void package(const int argc, char* argv[]) {
    // TODO
}

void fetch(const int argc, char* argv[]) {
    // TODO
}

void add(const int argc, char* argv[]) {
    CHECK((argc < 2), man::add)

    string lib_name;
    string lib_version;
    bool is_localy_provided = false;

    int max = argc > 3 ? 3 : argc;
    for (int i=1; i<max; i++) {
        if (string("--local").compare(argv[i]) == 0) {
            is_localy_provided = true;
        }
        else if (lib_name.empty()) {
            lib_name = argv[i];
        }
        else {
            lib_version = argv[i];
        }
    }

    auto lib_cfg = lib_configs[lib_name];
    auto commands = lib_configs["command"];

    string cfg_string =
        "version = '"+lib_version+"'\n"
        "command = [";

    commands->as<toml::Array>()->foreach([&cfg_string] (toml::Node* el) {
            cfg_string.append("'" + el->as<toml::String>()->_data + "'," );
        });
    cfg_string.back() = ']';

    auto cfg = toml::parse(cfg_string);

    //auto libs_cfg = lib_configs();
    //auto lib_cfg = libs_cfg[lib_name];
    //auto commands = lib_cfg["command"];
    //auto config = project_config();

    project_config.set_if_not("dependencies", new toml::Table);
    auto dependencies = project_config["dependencies"]->as<toml::Table>();
    dependencies->set(lib_name, &cfg);

    std::ofstream(root / "spear.toml") << project_config;
    if(argc > 4)
        add(argc - 2, argv+2);
}

void spear(const int argc, char* argv[]) {
    CHECK((argc < 2), man::spear)
    string argv1(argv[1]);
    find_global_config();

    if (argv1 == "new") {
        new_project(argc - 1, argv + 1);
        return;
    }

    find_root();
    find_project_config();
    find_lib_configs();

    if (argv1 == "run")
        run(argc - 1, argv+1);

    else if (argv1 == "build")
        build(argc - 1, argv+1);

    else if (argv1 == "clean")
        clean();

    else if (argv1 == "package")
        package(argc - 1, argv+1);

    else if (argv1 == "fetch")
        fetch(argc - 1, argv+1);

    else if (argv1 == "add")
        add(argc - 1, argv+1);

    else if (argv1 == "enable")
        enable_feature(argc - 1, argv+1);

    else if (argv1 == "get_name")
        std::cout << project_config["project.name"] << std::endl;

    else
        std::cout << man::spear << std::endl;

}

void enable_feature(const int argc, char **argv) {
    CHECK( (argc < 3), man::enable_feature);

    string lib = argv[1];
    if (!project_config["dependencies"]->as<toml::Table>()->_data.contains(lib)) {
        std::cout << "Error: the library '" << lib << "' is not added to this project" << std::endl
                  << "You may want to run 'spear add " << lib << "' first" << std::endl;
        return;
    }

    for (int i = 2; i < argc; i++) {
        string feature = argv[i];

        lib_configs[lib]->as<toml::Table>()
            ->get(feature)->as<toml::Table>()
            ->get("command")->as<toml::Array>()
            ->foreach([&lib]
            (toml::Node* command) {
                // TODO
                project_config[lib]->as<toml::Table>()
                    ->get("command")->as<toml::Array>()->_data
                    .push_back(command);
            });
    }

}
