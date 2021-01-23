# Using gRPC for the robot's debug terminal

This repository contains a suggestion on how we could write a debug interface for [CVRA](https://cvra.ch) robots.
If this proposal is validated by the club's other engineers, I will spend some time properly integrating into [cvra/robot-software](https://github.com/cvra/robot-software).

## Context

When debugging the robots, it is very common to write short snippets of code to expose some part of the system.
For example, one might want a way to make the robot execute a single action, or a way to test an idea.
Those snippets have usually a short lifetime (generally one edition of the contest), and are loosely defined.

In the past, we used a "shell" implemented over a serial port to control those snippets.
A developer who wanted to add a command could simply register a new command name, write a function to hold the code they wanted, and they could then invoke it at the commandline.
They could also print debug information, which would show up on the same serial port.

While migrating the robot software to Linux, we lost this "debug shell" capability.
Some of it was replaced by a touchscreen, which is great for things such as showing sensor values, or manually controlling actuators for debug. 
However, some use cases are not very well served by the touchscreen, either because they are too complex for a small screen or because writing a screen for the UI takes quite some time.

When running on Linux, it is quite unlikely that you would want to connect to a serial port in order to issue commands to the robot.
Running commands through the network requires no particular hardware, and can even be done through Wi-Fi.
The downside is that the developer's machine requires a client software to interact with the robot, although it could be as simple as `telnet`.

## Reasons to use gRPC

Going with a heavier client allows us to have richer capabilities compared to just text.
We could imagine that the client would want to fetch images, or make plots, both of which are harder to do through simple text interfaces.
We can also easily leverage the richer tools available, such as the `readline` module in Python for easier command line editing and persisting history to a file.

A proper RPC mechanism is also interesting for communication between programs, rather than communication intended for a human.
For example, this year we have a computer vision system running on a separate computer, which will need to send its results to the robot.
Re-using the same RPC system used for debugging is more convenient than implementing a one-off solution over again.

gRPC also provides out of the box features that we would have to implement ourselves otherwise.
The automatic encoding and decoding of messages, available in C++, Python and other makes it easy to start sending new type of information.
Automatic reconnection to a backend is very useful, as debug sessions will persist even if the robot's software gets restarted for an update. 
The client and server are able to negotiate on what they support, returning a proper error code in case a command is not implemented or does not parse succesfully.

## Alternatives considered

The first implementation of the debug terminal for Linux used raw TCP sockets to expose a debug terminal.
Users would connect with `telnet` and send commands, which would get parsed and executed on the robot.
While this worked, it required a lot of boilerplate code to handle sockets in a somewhat stable way, and did not really take full advantage of the "better" link that we had between the robot and a laptop.
In particular, features such as commandline history were a pain to implement, and caused issues when multiple client connections occured.

Another option was to run a web server inside the robot, and have each features be a particular request path or web page.
This would have enabled richer interfaces, and did not require any custom client software.
The main downside of this solution was the complexity that it created, as web frameworks in C++ are quite verbose.
If we want a web interface for some future need, an option would be to have an intermediate software to render web pages from data obtained via gRPC.
That way, we could use Python to implement the web application, and the C++ code would expose a simpler interface.

One last option would have been to use another RPC mechanism, such as JSON-RPC or Thrift.
Those RPC systems do not provide any relevant capabilties compared to gRPC, and are a bit more niche.
In addition, the team is already familar with Protobuf, the encoding mechanism of gRPC, which is already used in the robot.

## Proof of concept 

The proof of concept implements the two parts that would be required for this system (client and server).
The server is written in C++ while the client is written in Python.
While the server language is fixed (the robot uses C++), the client could be rewritten in any other language if we feel it is better.

The proof of concept implements two commands: one to dump a parameter tree (from `lib/parameter`) and one to get the position of the robot (always returns a fixed value).
The code is reasonnably compact, especially if we consider that a large part of it is boilerplate, and would not be needed when implementing new commands.    
To show this, the server code is separated between `server.cc` (boilerplate) and `commands.cc` (actual code a dev would need to write for commands).

To run the examples, you will need [Bazel](https://bazel.build/), Python and a C++ compiler.
Bazel is simply used to easily download gRPC and other dependencies, but is not a requirement.
We could integrate just as well in the existing CMake build system. 

### Running the server

To run the server, simply run the following command:

```shell
$ bazel run :server
```

### Running the client

You will need Python 3, and Bazel as well.
To run the client, run the following command 

```
$ bazel run :client

# Shows the possible commands
cvra>help
``` 

You can view the different options by running `bazel run :client -- -h`.
