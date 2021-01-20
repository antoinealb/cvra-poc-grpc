#!/bin/bash
# Generates Python gRPC files, as for some reason the Bazel integration does
# not work

set -eu

python -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. \
    debug_service.proto
