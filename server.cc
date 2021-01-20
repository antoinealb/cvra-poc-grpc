#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include <grpcpp/grpcpp.h>

#include "parameter.h"
#include "commands.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

ABSL_FLAG(std::string, server_address, "127.0.0.1:50051", "Server bind address");

parameter_namespace_t root;

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
