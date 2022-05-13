#pragma once

#include <string>


enum class random_string_type
{
    alphabetic,
    numeric,
};

std::string get_random_string(std::size_t size, random_string_type type);
std::string generate_uuid_v4();