#pragma once
#include <string_view>


enum class Query
{
    CHECK_EMAIL,
    CHECK_LOGIN,
    REGISTER_USER,
    LOGIN_USER_BY_PASS,
    LOGIN_USER_BY_TOKEN,
    GET_USER_INFO,
    GET_ALL_USER_IMAGES,
    GET_ALL_MARKER_IMAGES,
    GET_ALL_MARKERS,
    GET_ALL_PUSH_TOKENS,
    ADD_MARKER,
    QUERY_COUNT
};

class QueryManager
{
public:
    static std::string_view Get(Query type);
};