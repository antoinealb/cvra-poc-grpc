#pragma once

#include "debug_service.grpc.pb.h"
#include <grpcpp/grpcpp.h>

class DebugServiceImpl final : public DebugService::Service {
    grpc::Status ListParameters(grpc::ServerContext* context,
                                const ParameterListRequest* request,
                                ParameterListResponse* response) override;
    grpc::Status GetPosition(grpc::ServerContext* context,
                             const GetPositionRequest* request,
                             GetPositionResponse* response) override;
};
