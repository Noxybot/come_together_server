#include "tests.h"

#include <thread>
#include "../grpc/come_together.grpc.pb.h"
#include <grpc++/create_channel.h>
#include <plog/Log.h>

using namespace ComeTogether;
std::unique_ptr<MainEndpoint::Stub> test_stub;
void RUN_ALL_TESTS_IMPL();
template <class ResultType, class MemberT, class GetRequestType = std::nullptr_t, class PostCall = std::nullptr_t>
void test(std::string_view test_name, MemberT member, GetRequestType get_request = nullptr, PostCall post_call = nullptr);
void RUN_ALL_TESTS()
{
    std::thread {RUN_ALL_TESTS_IMPL}.detach();
}
void RUN_ALL_TESTS_IMPL()
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    test_stub = MainEndpoint::NewStub(
    grpc::CreateChannel("127.0.0.1:53681", grpc::InsecureChannelCredentials()));
    test<check_response>("Check email", &decltype(test_stub)::element_type::Check, []
    {
        check_request req;
        req.set_check_type(check_request_type_EMAIL);
        req.set_data("test@mail.com");
        return req;
    });
    test<check_response>("Check login", &decltype(test_stub)::element_type::Check, []
    {
        check_request req;
        req.set_check_type(check_request_type_LOGIN);
        req.set_data("vasya_poopckin");
        return req;
    });
    test<register_response>("Register user", &decltype(test_stub)::element_type::RegisterUser, []
    {
        register_request req;
        auto user_i = new user_info;
        user_i->set_email("test123@test.ua");
        user_i->set_login("vasya_poopckin");
        user_i->set_password("qweASD123");
        user_i->set_first_name("Vasya");
        user_i->set_other_info_json("{}");
        req.set_allocated_info(user_i);
        return req;
    });
    test<login_response>("Login user OK", &decltype(test_stub)::element_type::LoginUser, []
    {
        login_request req;
        req.set_login("vasya_poopckin");
        req.set_password("qweASD123");
        return req;
    }, [&](const login_response& res)
    {
        grpc::ClientContext ctx;
        access_token token;
        auto test_token = res.access_token();
        token.set_token(test_token);
        auto client_reader = test_stub->SubscribeToEvents(&ctx, token);
        event evt;
        test_stub.reset();
        while(client_reader->Read(&evt))
        {
            PLOG_DEBUG << evt.Utf8DebugString();
                        //client_reader->Finish();
            test_stub.reset();
            break;
        };
        
        //ctx.TryCancel();
       // client_reader->Finish();
        PLOG_DEBUG << "TEST FINISHED!!!!!!!!!\n";
    });
   /* test<login_response>("Login user WRONG PASS", &decltype(test_stub)::element_type::LoginUser, []
    {
        login_request req;
        req.set_login("vasya_poopckin");
        req.set_password("qweASD123$");
        return req;
    });
    test<login_response>("Login user WRONG LOGIN", &decltype(test_stub)::element_type::LoginUser, []
    {
        login_request req;
        req.set_login("vasya_poopckin1");
        req.set_password("qweASD123");
        return req;
    });*/
    //test<ask_token_response>("Ask token", &decltype(test_stub)::element_type::AskToken, []
    //{
    //    ask_token_request req;
    //    req.set_email("eduard.voronkin@nure.ua");
    //    return req;
    //});

}

template <class ResultType, class MemberT, class GetRequestType, class PostCall>
void test(std::string_view test_name, MemberT member, GetRequestType get_request, PostCall post_call)
{
    std::string log_res;
    grpc::ClientContext ctx;
    ResultType res;
    log_res.append("\nTEST: ").append(test_name).append("\n");
    if constexpr (!std::is_same_v<GetRequestType, std::nullptr_t>)
    {
        const auto req = get_request();
        log_res.append("\nIN:\n").append(req.Utf8DebugString()).append("\n");
        (*test_stub.*member)(&ctx, req, &res);
    }
    else
        (*test_stub.*member)(&ctx, {}, &res);
    log_res.append("OUT:\n").append(res.Utf8DebugString()).append("\n");
    PLOG_DEBUG << log_res;
    if constexpr (!std::is_same_v<PostCall, std::nullptr_t>)
        post_call(res);
}
