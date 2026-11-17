#include "config_parser.h"
#include <gtest/gtest.h>
#include <string>

TEST(parser_test, read_file){
    std::string filepath = "input/unit_test.cfg";
    ConfigParser parser;
    parser.load(filepath);
    assert(true);
}

TEST(parser_test, read_string){
    std::string filepath = "input/unit_test.cfg";
    ConfigParser parser;
    parser.load(filepath);
    
    std::string enable = parser.read_string("transposition/enable");
    ASSERT_EQ(enable, "no");
}

TEST(parser_test, read_int){
    std::string filepath = "input/unit_test.cfg";
    ConfigParser parser;
    parser.load(filepath);
    
    int num = parser.read_int("transposition/num");
    ASSERT_EQ(num, 2);
}