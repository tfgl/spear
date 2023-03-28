#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <istream>
#include <regex>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <functional>
#include <filesystem>
#include <optional>

namespace toml {
namespace fs = std::filesystem;
template<class T>
using vec = std::vector<T>;
template<class K, class V>
using map = std::unordered_map<K, V>;
using str = std::string;
using cstr = const std::string;
using sstr = std::stringstream;

cstr BEGIN  = "^ *";
cstr END    = " *(#.*)?$";
cstr ID     = "[a-zA-Z_][a-zA-Z0-9_]*";
cstr ASSIGN = " *= *";
cstr TABLE_KEY = "[a-zA-Z_][a-zA-Z0-9_.]*";

struct Node;
struct String;
struct Number;
struct Array;
struct Table;

struct Node {
    virtual str type() = 0;
    virtual bool is(str type) = 0;
    template<class T>
    T* as() {
        return (T*)this;
    }

    friend std::ostream& operator<<(std::ostream& os, Node& n);
    friend std::ostream& operator<<(std::ostream& os, Node* n);
    friend std::ostream& operator<<(std::ostream& os, std::optional<Node*> n);
};

struct String: Node {
    str _data;

    String(str data);
    str type();
    bool is(str type);

    static void parse(str& line, sstr&& data, Table& root_table);

    friend std::ostream& operator<<(std::ostream& os, const String& n);
    friend std::ostream& operator<<(std::ostream& os, String* n);
};

struct Number: Node {
    double _data;

    Number(double data);
    Number(str data);
    str type();
    bool is(str type);

    static void parse(str& line, sstr&& data, Table& root_table);

    friend std::ostream& operator<<(std::ostream& os, const Number& n);
    friend std::ostream& operator<<(std::ostream& os, Number* n);
};

struct Array: public Node {
    vec<Node*> _data;

    Array();
    Array(vec<Node*> data);
    void push(Node* data);
    Node& operator[](size_t index);
    void foreach(std::function<void(Node*)> f);
    str type();
    bool is(str type);

    static void parse(str& line, sstr&& data, Table& root_table);

    friend std::ostream& operator<<(std::ostream& os, const Array& n);
    friend std::ostream& operator<<(std::ostream& os, Array* n);
};

struct Table: public Node {
    map<str, Node*> _data;

    Table();
    Table(map<str, Node*> data);
    std::optional<Node*> operator[](const str& key);
    std::optional<Node*> get(const str& key);
    Node* set(const str& key, Node* value);
    Node* set_if_not(const str& key, Node* value);
    Node* value_or(str const& key, Node* value);
    void foreach(std::function<void(str const&, Node*)> f);
    str type();
    bool is(str type);
    bool contains(str key);
    bool is_ref_table() const;

    friend std::ostream& operator<<(std::ostream& os, const Table& n);
    friend std::ostream& operator<<(std::ostream& os, Table* n);

    static void parse(str& line, sstr&& data, Table& root_table);
};

inline Table parse(sstr&& data) {
    static map<str, std::function<void(str&, sstr&&, Table&)>> parse_map;
    parse_map.insert_or_assign(BEGIN+"\\["+TABLE_KEY+"\\]" +END, Table::parse);
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"\\[.+\\]"  +END, Array::parse);
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"[0-9]+"    +END, Number::parse);
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"\".+\""    +END, String::parse);
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"'.+'"      +END, String::parse);

    Table root_table;

    str line;
    while(std::getline(data, line)) {
        for (auto& [k, v]: parse_map) {
            std::regex pattern(k);
            if (std::regex_match(line, pattern)) {
                v(line, std::move(data), root_table);
                break;
            }
        }
    }

    return root_table;
}

inline Table parse(fs::path path) {
    auto fstr = std::ifstream(path);
    sstr data;
    data << fstr.rdbuf();
    fstr.close();
    return parse(std::move(data));
}

inline Table parse(const str& data) {
    return parse(std::stringstream(data));
}

inline Table parse(str&& data) {
    return parse(std::stringstream(data));
}
}
