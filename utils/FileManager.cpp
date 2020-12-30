#include "FileManager.h"
#include <algorithm>
#include <random>
#include <fstream>
#include <vector>
#include <iostream>
#include <filesystem>

static const std::string folder = "images/";
static const std::size_t name_size = 16;

std::vector<char> charset()
{
    //Change this to suit
    return std::vector<char>( 
    {'0','1','2','3','4',
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
    });
};    

// given a function that generates a random character,
// return a string of the requested length
template <class Functor>
std::string random_string( size_t length, Functor rand_char )
{
    std::string str(length,0);
    std::generate_n(str.begin(), length, rand_char);
    return str;
}

 //0) create the character set.
    //   yes, you can use an array here, 
    //   but a function is cleaner and more flexible
const auto ch_set = charset();

//1) create a non-deterministic random number generator      
std::default_random_engine rng(std::random_device{}());

//2) create a random number "shaper" that will give
//   us uniformly distributed indices into the character set
std::uniform_int_distribution<> dist(0, ch_set.size()-1);

//3) create a function that ties them together, to get:
//   a non-deterministic uniform distribution from the 
//   character set of your choice.
char randchar()
{
    return ch_set[ dist(rng) ];
}

FileManager::FileManager()
{
    std::filesystem::create_directory(folder);
}

std::string FileManager::SaveFile(const std::string& file)
{
    auto file_name = random_string(name_size, randchar);
    std::ofstream f {folder + file_name, std::ios_base::binary};
    f << file;
    std::lock_guard<decltype(m_mtx)> _ {m_mtx};
    m_cache[file_name] = file;
    std::cout << "ImageManger: saved file with size=" << file.size() << ", file=" << folder + file_name << std::endl;
    return file_name;
}

std::string FileManager::GetFile(const std::string& file_name)
{
    std::lock_guard<decltype(m_mtx)> _ {m_mtx};
    const auto it = m_cache.find(file_name);
    std::string res;
    if (it == std::end(m_cache))
    {
        std::ifstream f {folder + file_name, std::ios_base::binary};
        res = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        m_cache[file_name] = res;
        std::cout << "ImageManger: got file from cache\n";
    }
    else
        res = it->second;
    std::cout << "ImageManger: got file with size=" << res.size() << ", file=" << folder + file_name << std::endl;
    return res;
}
