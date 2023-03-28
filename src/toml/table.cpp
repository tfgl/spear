#include "toml.h"
#include <cstddef>
#include <optional>
#include <utility>

using namespace toml;

vec<str> split(str const& s, char delim) {
    vec<str> tokens;

    size_t word_start = 0;
    for (auto itr=s.begin(); itr!=s.end(); itr++) {
        if (*itr == delim) {
            tokens.push_back(std::string(s.begin()+word_start, itr));
            word_start = itr - s.begin() + 1;
            itr++;
        }
        else if (itr == s.end()-1) {
            tokens.push_back(std::string(s.begin()+word_start, itr+1));
        }
    }

    return tokens;
}

str make_key(vec<str> keys) {
    if(keys.empty()) return "";

    str key;
    for (size_t i=0; i<keys.size()-1; i++) {
        key += keys[i] + ".";
    }
    return key + keys.back();
}

Table::Table() {}
Table::Table(map<str, Node*> data): _data(data) {}

std::optional<Node*> Table::operator[](str const& key) {
    return get(key);
}

std::optional<Node*> Table::get(const str& key) {
    auto keys = split(key, '.');

    if (keys.size() > 1) {
        auto next = get(keys[0]);
        if (next.has_value() && next.value()->is("Table")) {
            keys.erase(keys.begin(), keys.begin()+1);
            str left_key = make_key(keys);
            return next.value()->as<Table>()->get(left_key);
        }
    }

    if (_data.count(key) > 0) {
        return _data[key];
    }

    return std::nullopt;
}

Node* Table::set(const str& key, Node* value) {
    auto keys = split(key, '.');

    if (keys.size() == 1) {
        _data[key] = value;
        return value;
    }

    auto right_key = keys[0];
    if (!get(right_key).has_value()) {
        set(right_key, new Table);
    }

    auto child = get(right_key).value();
    if (!child->is("Table")) throw(key + " contain non table value at " + right_key);

    keys.erase(keys.begin(), keys.begin()+1);
    str left_key = make_key(keys);


    return child->as<Table>()->set(left_key, value);
}

Node* Table::set_if_not(const str& key, Node* value) {
    if (get(key).has_value())
        return get(key).value();
    return set(key, value);
}

Node* Table::value_or(str const& key, Node* value) {
    auto get_result = get(key);
    if (get_result.has_value()) {
        return get_result.value();
    }
    return set(key, value);
}

void Table::foreach(std::function<void(str const&, Node*)> f) {
    for (auto& [k, v]: _data) f(k, v);
}

str Table::type() { return "Table"; }
bool Table::is(str type) { return type == "Table"; }

bool Table::contains(str key) {
    return get(key).has_value();
}

bool Table::is_ref_table() const {
    for (auto [k, v]: _data) {
        if (!v->is("Table"))
            return false;
    }
    return true;
}

std::ostream& toml::operator<<(std::ostream& os, Table const& n) {
    static vec<str> keys;
    vec<std::pair<str, Node*>> non_tables;

    for (auto [k, v]: n._data) {
        if ( !v->is("Table") ) {
            non_tables.push_back(std::make_pair(k, v));
            continue;
        }
        keys.push_back(k);
        auto t = v->as<Table>();
        os << *t;
    }

    if (!n.is_ref_table()) {
        if(!keys.empty()) {
            str table_key = make_key(keys);
            os << "[" + table_key + "]\n";
        }

        for (auto pair: non_tables) {
            auto n = pair.second;
            auto k = pair.first;

            if ( n->is("String") ) {
                os << k << " = " << *n->as<String>() << "\n";
            }
            else if ( n->is("Number") ) {
                os << k << " = " << *n->as<Number>() << "\n";
            }
            else if ( n->is("Array") ) {
                os << k << " = " << *n->as<Array>() << "\n";
            }
        }

        os << "\n";
    }

    if(!keys.empty())
        keys.pop_back();

    return os;
}

std::ostream& toml::operator<<(std::ostream& os, Table* n) {
    os << *n;
    return os;
}

void Table::parse(str &line, sstr &&data, Table &root_table) {
    std::smatch matches;

    // exctract maximum key from line to matches
    {
        std::regex re(TABLE_KEY);
        std::regex_search(line, matches,re);
    }

    // create a new table in root_table with the maximum key (key.to.something)
    str const& key = matches[0].str();
    Table* t = root_table.value_or(key, new Table)->as<Table>();

    // create refrencing table (empty table containing only other tables)
    {
        auto const& keys = split(key, '.');
        Table* parent_table = &root_table;

        for (size_t i=0; i<keys.size(); i++) {
            str const& sub_key = keys[i];

            if (i < keys.size() - 1) {
                parent_table = parent_table->value_or(sub_key, new Table)->as<Table>();
            }
            else {
                parent_table->set_if_not(sub_key, t);
            }
        }

    }

    static map<str, std::function<void(str&, sstr&&, Table&)>> parse_map;
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"\\[.+\\]"  +END, Array::parse);
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"[0-9]+"    +END, Number::parse);
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"\".+\""    +END, String::parse);
    parse_map.insert_or_assign(BEGIN+ID+ASSIGN+"'.+'"      +END, String::parse);

    str field;
    std::regex EOL("^ *$");
    while (std::getline(data, field) && !std::regex_match(field, EOL)) {
        for (auto& [k, v]: parse_map) {
            std::regex pattern(k);
            if (std::regex_match(field, pattern)) {
                v(field, std::move(data), *t);
                break;
            }
        }
    }
}

