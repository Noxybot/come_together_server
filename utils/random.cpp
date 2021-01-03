#include "random.h"

#include <cassert>
#include <random>
#include <sstream>


static char charset[] = {
    '0','1','2','3','4',
    '5','6','7','8','9',
    'A','B','C','D','E','F',
    'G','H','I','J','K',
    'L','M','N','O','P',
    'Q','R','S','T','U',
    'V','W','X','Y','Z',
    'a','b','c','d','e','f',
    'g','h','i','j','k',
    'l','m','n','o','p',
    'q','r','s','t','u',
    'v','w','x','y','z'
    };

static std::default_random_engine rng(std::random_device{}());
static std::uniform_int_distribution<> dist(0, sizeof(charset) / sizeof(charset[0]) - 1);

static char rand_char()
{
    return charset[dist(rng)];
}

static char rand_number()
{
    return charset[dist(rng) % 9];
}

template <class Functor>
static std::string random_string(std::size_t length, Functor rand_char )
{
    std::string str(length,0);
    std::generate_n(str.begin(), length, rand_char);
    return str;
}

std::string get_random_string(std::size_t size, random_string_type type)
{
    switch (type)
    {
        case random_string_type::alphabetic:
        return random_string(size, rand_char);
        case random_string_type::numeric:
        return random_string(size, rand_number);
    default:
        assert(false);
        return {};
    }
}

namespace uuid {
    static std::random_device              rd;
    static std::mt19937                    gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
}
std::string generate_uuid_v4()
{
        std::stringstream ss;
        int i;
        ss << std::hex;
        for (i = 0; i < 8; i++) {
            ss << uuid::dis(uuid::gen);
        }
        ss << "-";
        for (i = 0; i < 4; i++) {
            ss << uuid::dis(uuid::gen);
        }
        ss << "-4";
        for (i = 0; i < 3; i++) {
            ss << uuid::dis(uuid::gen);
        }
        ss << "-";
        ss << uuid::dis2(uuid::gen);
        for (i = 0; i < 3; i++) {
            ss << uuid::dis(uuid::gen);
        }
        ss << "-";
        for (i = 0; i < 12; i++) {
            ss << uuid::dis(uuid::gen);
        };
        return ss.str();
    }