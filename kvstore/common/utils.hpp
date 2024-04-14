#ifndef COMMON_UTILS_HPP
#define COMMON_UTILS_HPP

#include <algorithm>
#include <cctype>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

// Random utils.

// Helper function to split strings by whitespace.
std::vector<std::string> split(const std::string& s, char delim = ' ');

// Checks if a string is a (positive) number
bool is_number(const std::string& s);

// Convert a string to upper/lower case, because cpp doesn't have that
// functionality yet... this is pretty fragile, it only works on ASCII
// characters right now (sorry Unicode!)
std::string to_upper(const std::string& s);
std::string to_lower(const std::string& s);

#endif /* end of include guard */