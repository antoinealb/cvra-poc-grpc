#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/grpcpp.h>

#include "debug_service.grpc.pb.h"
#include "parameter.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

ABSL_FLAG(std::string, server_address, "127.0.0.1:50051", "Server bind address");

parameter_namespace_t root;

Status get_ns_content(parameter_namespace_t* ns, ParameterNamespaceContent* res)
{
    res->set_name(ns->id);

    for (parameter_t* p = ns->parameter_list; p != nullptr; p = p->next) {
        auto* val = res->add_values();
        val->set_name(p->id);

        if (!parameter_defined(p)) {
            return {StatusCode::FAILED_PRECONDITION, "parameter has no value"};
        }

        switch (p->type) {
            case _PARAM_TYPE_SCALAR:
                val->set_scalar_value(parameter_scalar_get(p));
                break;
            case _PARAM_TYPE_INTEGER:
                val->set_integer_value(parameter_integer_get(p));
                break;
            case _PARAM_TYPE_BOOLEAN:
                val->set_bool_value(parameter_boolean_get(p));
                break;
            default:
                return {StatusCode::UNIMPLEMENTED, absl::StrFormat("unexpected parameter type %d", p->type)};
        }
    }

    for (auto* p = ns->subspaces; p != nullptr; p = p->next) {
        auto* children = res->add_children();
        Status s = get_ns_content(p, children);
        if (!s.ok()) {
            return s;
        }
    }

    return Status::OK;
}

class DebugServiceImpl final : public DebugService::Service {
    // This is typically what would be implemented to create a new command.
    Status ListParameters(ServerContext* context,
                          const ParameterListRequest* request,
                          ParameterListResponse* response) override
    {
        parameter_namespace_t* ns = &root;

        if (!request->path().empty()) {
            ns = parameter_namespace_find(&root, request->path().c_str());
            if (!ns) {
                return {StatusCode::NOT_FOUND, "invalid parameter namespace"};
            }
        }

        auto* content = response->add_contents();
        return get_ns_content(ns, content);
    }

    Status GetPosition(ServerContext* context, const GetPositionRequest* /*request*/, GetPositionResponse* response) override
    {
        response->set_x(42.f);
        response->set_y(60.f);
        response->set_a(3.14);
        return Status::OK;
    }
};

void RunServer()
{
    std::string server_address(absl::GetFlag(FLAGS_server_address));
    DebugServiceImpl service;

    ServerBuilder builder;

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

void SetupParameters()
{
    static parameter_t foo, bar, baz;
    static parameter_namespace_t myns;
    parameter_namespace_declare(&root, nullptr, "");
    parameter_namespace_declare(&myns, &root, "myns");
    parameter_boolean_declare_with_default(&baz, &root, "baz", true);
    parameter_integer_declare_with_default(&foo, &root, "foo", 42);
    parameter_scalar_declare_with_default(&bar, &myns, "bar", 42.);
}

int main(int argc, char** argv)
{
    absl::ParseCommandLine(argc, argv);
    SetupParameters();
    RunServer();

    return 0;
}
