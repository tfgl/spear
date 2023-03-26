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

struct Node {
    virtual str serialize() = 0;
    virtual str serialize(str key) = 0;
    virtual str type() = 0;
    virtual bool is(str type) = 0;
    template<class T>
    T* as() {
        return (T*)this;
    }
};

struct Comment: public Node {
    str _data;

    str serialize() { return "#" + _data; }
    str serialize(str key) { return "#" + _data; }
    str type() { return "Comment"; }
    bool is(str type) { return type == "Comment"; }

    friend std::ostream& operator<<(std::ostream& os, Comment n) { os << n._data; return os; }

    static std::pair<str, Comment*> parse(str& line, sstr&& data) {
        Comment* comment = new Comment;
        return std::make_pair("", comment);
    }
};

struct String: Node {
    str _data;

    String(str data): _data(data) {}

    str serialize() {
        return "'" + _data + "'";
    }

    str serialize(str key) {
        return key + " = '" + _data + "'";
    }

    str type() { return "String"; }
    bool is(str type) { return type == "String"; }

    friend std::ostream& operator<<(std::ostream& os, String n) { os << "'" << n._data << "'"; return os; }

    static std::pair<str, toml::String*> parse(str& line, sstr&& data) {
        std::regex re(ASSIGN);
        std::sregex_token_iterator p(line.begin(), line.end(), re, -1);
        str key = *p++;
        str value = *p;

        // TODO: change to remove only the bounding quotes (regex ?)
        value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
        value.erase(std::remove(value.begin(), value.end(), '\''), value.end());
        
        String* string = new String(value);
        return std::make_pair(key, string);
    }
};

struct Number: Node {
    double _data;

    Number(double data): _data(data) {}
    Number(str data) {_data = std::atof(data.c_str());}

    str serialize() {
        return std::to_string(_data);
    }

    str serialize(str key) {
        return key + " = " + std::to_string(_data);
    }

    str type() { return "Number"; }
    bool is(str type) { return type == "Number"; }

    friend std::ostream& operator<<(std::ostream& os, Number n) { os << n._data; return os; }

    static std::pair<str, Number*> parse(str& line, sstr&& data) {
        std::regex re(ASSIGN);
        std::sregex_token_iterator p(line.begin(), line.end(), re, -1);
        str key = *p++;
        str value = *p;

        Number* number = new Number(value);
        return std::make_pair(key, number);
    }
};

struct Array: public Node {
    vec<Node*> _data;

    Array() {}
    Array(vec<Node*> data): _data(data) {}

    void push(Node* data) {
        _data.push_back(data);
    }

    Node* operator[](size_t index) {
        return _data.at(index);
    }

    void foreach(std::function<void(Node*)> f) {
        for (auto& elt: _data) {
            f(elt);
        }
    }

    str serialize() {
        str serialized;

        for (int i=0; i<_data.size() - 1; i++) {
            serialized.append(_data[i]->serialize() + ", ");
        }
        serialized.append(_data.back()->serialize() + "]");

        return serialized;
    }

    str serialize(str key) {
        str serialized = key + " = [";

        for (int i=0; i<_data.size() - 1; i++) {
            serialized.append(_data[i]->serialize() + ", ");
        }
        serialized.append(_data.back()->serialize() + "]");

        return serialized;
    }

    str type() { return "Array"; }
    bool is(str type) { return type == "Array"; }

    friend std::ostream& operator<<(std::ostream& os, Array n) {
        for (int i=0; i<n._data.size(); i++) {
            Node* data = n._data[i];

            if ( data->is("String") )
                os << *(String*)data;
            else if ( data->is("Number") )
                os << *(Number*)data;

            if (i < n._data.size() -1)
                os << ", ";
        }
        return os;
    }

    static std::pair<str, Array*> parse(str& line, sstr&& data) {
        std::regex re(ASSIGN);
        std::sregex_token_iterator p(line.begin(), line.end(), re, -1);
        str key = *p++;
        str right = *p;

        Array* array = new Array;
        std::regex re2(", *");
        std::sregex_token_iterator p2(right.begin(), right.end(), re2, -1);
        std::sregex_token_iterator end;

        std::regex number("[0-9]+");
        while (p2 != end) {
            str token = *p2++;
            if (std::regex_match(token, number))
                array->push(new Number(token));
            else
                array->push(new String(token));
        }

        return std::make_pair(key, array);
    }
};

struct Table: public Node {
    map<str, Node*> _data;

    Table() {}
    Table(map<str, Node*> data): _data(data) {}

    Node* operator[](const str& key) {
        str key_copy(key);
        Node* n = this;
        char* next_key = std::strtok(key_copy.data(), ".");

        while (next_key != NULL) {
            n = n->as<Table>()->get(next_key);
            next_key = std::strtok(NULL, ".");
        }

        return n;
    }

    Node* get(const str& key) {
        return _data.at(key);
    }

    void set(const str& key, Node* value) {
        //_data.insert_or_assign(key, value);
        _data[key] = value;
    }

    void set_if_not(const str& key, Node* value) {
        if (!_data.contains(key))
            _data[key] = value;
    }

    void foreach(std::function<void(Node*)> f) {
        for (auto& [k, v]: _data) {
            f(v);
        }
    }

    str serialize() {
        str serialized;

        for (auto& [k, v]: _data) {
            serialized.append(v->serialize(k) + "\n");
        }

        return serialized;
    }

    str serialize(str key) {
        str serialized = "[" + key + "]\n";

        for (auto& [k, v]: _data) {
            serialized.append(v->serialize(k) + "\n");
        }

        return serialized;
    }

    str type() { return "Table"; }
    bool is(str type) { return type == "Table"; }

    friend std::ostream& operator<<(std::ostream& os, Table n) {
        for (auto [k, v]: n._data) {
            if ( v->is("Table") )
                os << "[" << k << "]\n" << *(Table*)v << "\n";
            else if ( v->is("String") )
                os << k << " = " << *(String*)v << "\n";
            else if ( v->is("Number") )
                os << k << " = " << *(Number*)v << "\n";
            else if ( v->is("Array") )
                os << k << " = " << *(Array*)v << "\n";
            else if ( v->is("Comment") )
                os << k << " = " << *(Comment*)v << "\n";
        }
        return os;
    }

    static std::pair<str, Table*> parse(str& line, sstr&& data) {
        std::regex re(ID);
        std::smatch matches;
        std::regex_search(line, matches,re);

        str key = matches[0].str();
        Table* t = new Table;


        static map<str, std::function<std::pair<str, Node*>(str&, sstr&&)>> parse_map;
        //parse_map.insert_or_assign("#.*"                       , Comment::parse);
        parse_map.insert_or_assign(BEGIN+"\\["+ID+"\\]"      +END, Table::parse);
        parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"\\[.+\\]"+END, Array::parse);
        parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"[0-9]+"  +END, Number::parse);
        parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"\".+\""  +END, String::parse);
        parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"'.+'"    +END, String::parse);

        str field;
        std::regex EOL("^ *$");
        while (std::getline(data, field) && !std::regex_match(field, EOL)) {
            for (auto& [k, v]: parse_map) {
                std::regex pattern(k);
                if (std::regex_match(field, pattern)) {
                    auto node = v(field, std::move(data));
                    t->set(node.first, node.second);
                    break;
                }
            }
        }
        return std::make_pair(key, t);
    }
};

inline Table parse(sstr&& data) {
    static map<str, std::function<std::pair<str, Node*>(str&, sstr&&)>> parse_map;
    //parse_map.insert_or_assign("#.*"                       , Comment::parse);
    parse_map.insert_or_assign(BEGIN+"\\["+ID+"\\]"      +END, Table::parse);
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"\\[.+\\]"+END, Array::parse);
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"[0-9]+"  +END, Number::parse);
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"\".+\""  +END, String::parse);
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"'.+'"    +END, String::parse);

    Table parsed;

    str line;
    while(std::getline(data, line)) {
        for (auto& [k, v]: parse_map) {
            std::regex pattern(k);
            if (std::regex_match(line, pattern)) {
                auto node = v(line, std::move(data));
                parsed.set(node.first, node.second);
                break;
            }
        }
    }

    return parsed;
}

inline Table parse(fs::path path) {
    auto fstr = std::ifstream(path);
    sstr data;
    data << fstr.rdbuf();
    fstr.close();
    return parse(std::move(data));
}

inline Table parse(str path) {
    return parse(fs::path(path));
}
}
