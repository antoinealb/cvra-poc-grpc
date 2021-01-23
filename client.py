#!/usr/bin/env python3
import grpc
import cmd
import debug_service_pb2_grpc
import debug_service_pb2
import argparse
import shlex
import os.path
import readline
import math


def dump_parameter_tree(content, indent=0):
    def i(width):
        return "  " * width

    print(i(indent) + content.name + ":")

    for p in content.values:
        print(i(indent + 1) + "{}: ".format(p.name), end="")

        if p.HasField("integer_value"):
            print(p.integer_value)
        elif p.HasField("scalar_value"):
            print("{:.4f}".format(p.scalar_value))
        elif p.HasField("bool_value"):
            print(p.bool_value)
        else:
            print("<unsupported in client>")

    for c in content.children:
        dump_parameter_tree(c, indent=indent + 1)


class DebugShell(cmd.Cmd):
    prompt = "cvra >"

    def __init__(self, stub, histfile):
        super().__init__()
        self.stub = stub
        self.histfile = histfile

    def preloop(self):
        if os.path.exists(self.histfile):
            readline.read_history_file(self.histfile)

    def postloop(self):
        readline.set_history_length(1000)
        readline.write_history_file(self.histfile)

    def do_parameter(self, arg):
        """
        List all parameters exposed by the robot.
        """
        p = argparse.ArgumentParser()
        p.add_argument("namespace", nargs="?")
        try:
            args = p.parse_args(shlex.split(arg))
        except SystemExit:
            return

        request = debug_service_pb2.ParameterListRequest()
        if args.namespace:
            request.path = args.namespace

        try:
            res = self.stub.ListParameters(request)
            dump_parameter_tree(res.contents[0])
        except grpc.RpcError as e:
            print("{}: {}".format(e.code(), e.details()))

    def do_parameter_set(self, arg):
        """
        Sets a single parameter, specifying its type.
        """
        p = argparse.ArgumentParser()
        p.add_argument("parameter")
        p.add_argument("-i", type=int, help="Integer value to set")
        p.add_argument("-f", type=float, help="Float value to set")
        p.add_argument("-b", type=int, help="Bool (1 or 0)")

        try:
            args = p.parse_args(shlex.split(arg))
        except SystemExit:
            return

        req = debug_service_pb2.SetParameterRequest()
        req.value.name = args.parameter
        if args.i is not None:
            req.value.integer_value = args.i
        elif args.f is not None:
            req.value.scalar_value = args.f
        elif args.b is not None:
            req.value.bool_value = args.b
        else:
            print("Must specify one parameter value!")
            return

        try:
            self.stub.SetParameter(req)
        except grpc.RpcError as e:
            print("{}: {}".format(e.code(), e.details()))

    def do_position(self, arg):
        """
        Gets the position of the robot.
        """
        try:
            res = self.stub.GetPosition(debug_service_pb2.GetPositionRequest())
            print("{:.3f}, {:.3f}, {:.3f}".format(res.x, res.y, math.degrees(res.a)))
        except grpc.RpcError as e:
            print("{}: {}".format(e.code(), e.details()))

    def do_exit(self, *args):
        # signals the shell to abort
        return True

    def do_EOF(self, *args):
        return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--server", "-s", default="localhost:50051", help="Address of the gRPC server"
    )
    parser.add_argument(
        "--history_file", default="~/.cvra_history", help="History file"
    )
    args = parser.parse_args()

    history_file = os.path.expanduser(args.history_file)

    channel = grpc.insecure_channel(args.server)
    stub = debug_service_pb2_grpc.DebugServiceStub(channel)

    DebugShell(stub, history_file).cmdloop()
