#pragma once

#include "toml.h"

toml::parse_result project_config();
toml::parse_result global_config();
toml::parse_result libs_config();

void new_project(const int argc, char* argv[]);
void build(const int argc, char* argv[]);
void run(const int argc, char* argv[]);
void package(const int argc, char* argv[]);
void fetch(const int argc, char* argv[]);
void add(const int argc, char* argv[]);
void enable_feature(const int argc, char* argv[]);
void spear(const int argc, char* argv[]);
