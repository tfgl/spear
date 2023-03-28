#include "toml.h"

using namespace toml;

String::String(str data): _data(data) {}

str String::type() {
    return "String";
}

bool String::is(str type) {
    return type == "String";
}

void String::parse(str &line, sstr &&data, Table &root_table) {
    std::regex re(ASSIGN);
    std::sregex_token_iterator p(line.begin(), line.end(), re, -1);
    str key = *p++;
    str value = *p;

    // TODO: change to remove only the bounding quotes (regex ?)
    value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
    value.erase(std::remove(value.begin(), value.end(), '\''), value.end());
    
    root_table.set_if_not(key, new String(value));
}


std::ostream& toml::operator<<(std::ostream& os, String const& n) {
    os << "'" << n._data << "'";
    return os;
}

std::ostream& toml::operator<<(std::ostream& os, String* n) {
    os << "'" << n->_data << "'";
    return os;
}
