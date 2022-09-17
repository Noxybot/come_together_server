#include "Server.h"
#include "tests/tests.h"
#include <fstream>

#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Appenders/ColorConsoleAppender.h>

#include <come_together.grpc.pb.h>
#include <grpcpp/create_channel.h>

#include <plog/Log.h>

#include <mongocxx/database.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <mongocxx/client.hpp>

#include <mongocxx/uri.hpp>
#include <plog/Log.h>

#include  <random>
#include  <iterator>
using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
const auto db_name = "come_together";
const auto users_collection_name = "users";
const auto markers_collection_name = "markers";
const auto chats_collection_name = "chats";

namespace CT = come_together_grpc;

template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}
template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen);
}
int main() try
{
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::verbose, &consoleAppender);
    /*const auto dima = CT::VerificationService::NewStub(grpc::CreateChannel("25.80.58.244:8080", grpc::InsecureChannelCredentials()));
    std::ifstream img{ "img.jpg", std::ios_base::binary };
    std::ifstream img1{ "img2.jpg", std::ios_base::binary };

    grpc::ClientContext ctx;
    CT::validate_photo_response response;
    const auto writer = dima->ValidatePhoto(&ctx, &response);
    while (true) {
        std::string buffer(1024, 0); //reads only the first 1024 bytes
        std::string buffer1(1024, 0); //reads only the first 1024 bytes
        bool first_complete = false, second_complete = false;
        if (img.read(buffer.data(), buffer.size()).eof())
            first_complete = true;
        if (img1.read(buffer1.data(), buffer1.size()).eof())
            second_complete = true;

        CT::validate_photo_request req;
        req.set_user_photo(std::move(buffer1));
        req.set_validation_photo(std::move(buffer));
        req.set_user_photo_complete(second_complete);
        req.set_validation_photo_complete(first_complete);
        req.set_gestue(CT::validate_photo_request_gestue_type_OK);
        writer->Write(req);
        if (first_complete && second_complete)
        {
            std::cout << "all complete, finish writing\n";
            writer->Finish();
            break;
        }
    }
    std::cout << "response: " << response.Utf8DebugString();
    return 0;
    if (!img.is_open())
    {
        return -1;
    }*/

    /*mongocxx::instance m_instance;
    mongocxx::pool m_pool;
    std::vector<std::string> f_names{ "Vasya", "Dima", "Eduard", "Tamara", "Alisa", "Kostya" };
    std::vector<std::string> s_names{ "Poopkin", "Shevchenko", "Voronkin", "Glushenkova", "Levadna", "Kirov" };
    std::vector<std::vector<std::string>> subcats
    {
        {
            "cycling", "football", "gym", "running", "other"
        },
        {
            "cinema", "theater", "excursion", "gallery", "other"
        },
        {
            "cafe", "restaurant", "other"
        },
        {
            "bar", "night_club", "hookah", "other"
        },
        {
            "walk", "concert", "quest", "zoo", "other"
        },
    };
    const auto entry = m_pool.acquire();
    auto users_collection = (*entry)[db_name][users_collection_name];

    for (int i = 0; i < f_names.size(); ++i)
    {
        std::time_t curr_time = time(0);
        std::time_t ten_years = 365 * 12 * 30 * 24 * 60;
        std::time_t rand_date = (curr_time - (std::rand() * RAND_MAX + std::rand())) % ten_years;
        tm* ltm = std::localtime(&rand_date);
        std::cout << ltm->tm_year + 1900 << " " << ltm->tm_mon + 1 << " " << ltm->tm_mday << std::endl;
        const auto doc = document{} << "email" << f_names[i] + "." + s_names[i] + "@nure.ua"
            << "family_name" << s_names[i] << "given_name" << f_names[i]
            << "birth_date" << std::to_string(ltm->tm_year + 1920) + "." + std::to_string(ltm->tm_mon + 1) + "." +
            std::to_string(ltm->tm_mday)
            << "gender" << (i % 2 ? "male" : "female")
            << "other_info" << open_document

			<< "sport" << open_array << *select_randomly(subcats[0].begin(), subcats[0].end()) << close_array
			<< "watch" << open_array << *select_randomly(subcats[1].begin(), subcats[1].end()) << close_array
			<< "eat" << open_array << *select_randomly(subcats[2].begin(), subcats[2].end()) << close_array
			<< "night_life" << open_array << *select_randomly(subcats[3].begin(), subcats[3].end()) << close_array
			<< "other" << open_array << *select_randomly(subcats[4].begin(), subcats[4].end()) << close_array
			<< close_document
            << finalize;
        users_collection.insert_one(doc.view());
    }*/
    Server srv;
   // RUN_ALL_TESTS();
    srv.Start();
    return 0;
}
catch (std::exception const &e)
{
    LOG_ERROR << e.what() << std::endl;
    return -1;
}
catch (...)
{
    LOG_ERROR << "Unknown exception" << std::endl;
}
