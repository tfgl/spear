#include "toml.h"

using namespace toml;
 
std::ostream& toml::operator<<(std::ostream& os, Node& n) {
    if (n.is("Table")) {
        os << n.as<Table>();
    }
    else if (n.is("String")) {
        os << n.as<String>();
    }
    else if (n.is("Number")) {
        os << n.as<Number>();
    }
    else if (n.is("Array")) {
        os << n.as<Array>();
    }
    else {
        os << "<Not a valid toml type>";
    }
    return os;
}

std::ostream& toml::operator<<(std::ostream& os, Node* n) {
    os << *n;
    return os;
}

std::ostream& toml::operator<<(std::ostream& os, std::optional<Node*> n) {
    os << *n;
    return os;
}
