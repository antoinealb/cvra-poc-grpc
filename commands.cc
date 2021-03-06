#include "parameter.h"
#include "commands.h"
#include <grpcpp/grpcpp.h>
#include "debug_service.grpc.pb.h"
#include "absl/strings/str_format.h"

using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

extern parameter_namespace_t root;

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

Status DebugServiceImpl::ListParameters(ServerContext* context,
                                        const ParameterListRequest* request,
                                        ParameterListResponse* response)
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

Status DebugServiceImpl::GetPosition(ServerContext* context, const GetPositionRequest* /*request*/, GetPositionResponse* response)
{
    response->set_x(42.f);
    response->set_y(60.f);
    response->set_a(3.14);
    return Status::OK;
}
Status DebugServiceImpl::SetParameter(grpc::ServerContext* context,
                                      const SetParameterRequest* request,
                                      SetParameterResponse* response)
{
    parameter_t* p;

    p = parameter_find(&root, request->value().name().c_str());

    if (!p) {
        return {StatusCode::NOT_FOUND, "unknown parameter"};
    }

    // Check that the type of the request is compatible with the type of the parameter
    if (p->type == _PARAM_TYPE_SCALAR && request->value().value_case() != ParameterValue::kScalarValue) {
        return {StatusCode::INVALID_ARGUMENT, "expected scalar"};
    }
    if (p->type == _PARAM_TYPE_INTEGER && request->value().value_case() != ParameterValue::kIntegerValue) {
        return {StatusCode::INVALID_ARGUMENT, "expected integer"};
    }
    if (p->type == _PARAM_TYPE_BOOLEAN && request->value().value_case() != ParameterValue::kBoolValue) {
        return {StatusCode::INVALID_ARGUMENT, "expected boolean"};
    }

    switch (p->type) {
        case _PARAM_TYPE_SCALAR:
            parameter_scalar_set(p, request->value().scalar_value());
            break;
        case _PARAM_TYPE_INTEGER:
            parameter_integer_set(p, request->value().integer_value());
            break;
        case _PARAM_TYPE_BOOLEAN:
            parameter_boolean_set(p, request->value().bool_value());
            break;
        default:
            return {StatusCode::UNIMPLEMENTED, absl::StrFormat("unexpected parameter type %d", p->type)};
    }

    return Status::OK;
}
