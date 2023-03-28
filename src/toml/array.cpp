#include "toml.h"

using namespace toml;

vec<Node*> _data;

Array::Array() {}
Array::Array(vec<Node*> data): _data(data) {}

void Array::push(Node* data) {
    _data.push_back(data);
}

Node& Array::operator[](size_t index) {
    return *_data.at(index);
}

void Array::foreach(std::function<void(Node*)> f) {
    for (auto& elt: _data) {
        f(elt);
    }
}

str Array::type() { return "Array"; }
bool Array::is(str type) { return type == "Array"; }

std::ostream& toml::operator<<(std::ostream& os, Array const& n) {
    os << "[";
    for (int i=0; i<n._data.size(); i++) {
        Node* data = n._data[i];

        if ( data->is("String") )
            os << *data->as<String>();
        else if ( data->is("Number") )
            os << *data->as<Number>();

        if (i < n._data.size() -1)
            os << ", ";
    }
    os << "]";
    return os;
}

std::ostream& toml::operator<<(std::ostream& os, Array* n) {
    os << *n;
    return os;
}

void Array::parse(str &line, sstr &&data, Table &root_table) {
    std::regex re(ASSIGN);
    std::sregex_token_iterator p(line.begin(), line.end(), re, -1);
    str key = *p++;
    str right = *p;
    right.assign(right.begin()+1, right.end()-1);

    Array *array = new Array;
    std::regex re2(", *");
    std::sregex_token_iterator p2(right.begin(), right.end(), re2, -1);
    std::sregex_token_iterator end;

    std::regex number("[0-9]+");
    while (p2 != end) {
        str token = *p2++;
        if (std::regex_match(token, number))
            array->push(new Number(token));
        else{
            token.assign(token.begin()+1, token.end()-1);
            array->push(new String(token));
        }
    }

    root_table.set_if_not(key, array);
}

