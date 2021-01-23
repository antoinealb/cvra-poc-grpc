load("@rules_proto_grpc//cpp:defs.bzl", "cpp_grpc_library")
load("@rules_proto_grpc//python:defs.bzl", "python_grpc_library")

proto_library(
    name = "debug_service",
    srcs = ["debug_service.proto"],
)

cpp_grpc_library(
    name = "debug_service_cc",
    deps = [":debug_service"],
)

python_grpc_library(
    name = "debug_service_python",
    deps = [":debug_service"],
)

cc_binary(
    name = "server",
    srcs = [
        "commands.cc",
        "commands.h",
        "server.cc",
    ],
    deps = [
        ":debug_service_cc",
        ":parameter",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "parameter",
    srcs = ["parameter.c"],
    hdrs = ["parameter.h"],
)

py_binary(
    name = "client",
    srcs = ["client.py"],
    deps = [":debug_service_python"],
)
