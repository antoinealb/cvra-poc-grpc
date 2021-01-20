# Proof of concept for CVRA debug terminal 

## Running the server

You will need [Bazel](https://bazel.build/) and a C++ compiler.
The rest is downloaded automatically during the build.
To run the server, simply run the following command:

```shell
$ bazel run :server
```

## Running the client

Unfortunately there are some issues building gRPC for Python 3.9.
Therefore the client's proto have to be built before being used.

```
pip install -r requirements.txt
./generate_client_proto.sh
./client.py
```
