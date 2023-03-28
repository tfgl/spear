#include "toml.h"

using namespace toml;

Number::Number(double data): _data(data) {}
Number::Number(str data) {_data = std::atof(data.c_str());}

str Number::type() {
    return "Number";
}

bool Number::is(str type) {
    return type == "Number";
}

void Number::parse(str &line, sstr &&data, Table &root_table) {
    std::regex re(ASSIGN);
    std::sregex_token_iterator p(line.begin(), line.end(), re, -1);
    str key = *p++;
    str value = *p;

    root_table.set_if_not(key, new Number(value));
}

std::ostream& toml::operator<<(std::ostream& os, Number const& n) {
    os << n._data;
    return os;
}

std::ostream& toml::operator<<(std::ostream& os, Number* n) {
    os << n->_data;
    return os;
}
