#pragma once
#include <sstream>
#include <string>

struct FooBar {
    int a_int {0};
    float a_float {0.0};
    double a_double {0.0};
    bool a_bool {false};
    std::string a_string;
    FooBar() {}
    ~FooBar() {}
    std::string toStr() const {
        std::stringstream ss;
        ss << "Foobar( "
           << "a_int=" << a_int << ", "
           << "a_float=" << a_float << ", "
           << "a_double= " << a_double << ", "
           << "a_bool= " << a_bool  << ", "
           << "a_string= " << a_string
           << " )"
        ;
        return ss.str();
    }
};