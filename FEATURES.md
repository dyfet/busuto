# Features

Busuto consists of a series of C++ 20 header files that typically are installed
under and included from /usr/include/busuto and linkable library code that is
focused on core functionality I use in server application development. Some of
these are templated headers only, and some require linking with the busuto
runtime library this package builds. These currently include:

NOTE: Busuto indeed can be built for Microsoft Windows if you use MingW32 with
posix support. Some features and functionality may be disabled when building
for Windows. There is no support for building Windows targets with MSVC.

## atomic.hpp

Atomic types and lockfree data structures. This includes lockfree stack,
buffer, and unordered dictionary implementations which are something like C#
ConcurrentStack, ConcurrentDictionary, and ConcurrentQueue. It also includes
an implementation of atomic\_ref that should be similar to the C++20 one.

## binary.hpp

This is the generic portable convertible flexible binary data array object
class and supporting utility conversions for/to B64 text, hex, and utf8
strings, that I have always wanted ever since using Qt QByteArray. This alone
drove my decision to migrate to C++20 or later for future projects.

## common.hpp

Some very generic, universal, miscellaneous templates and functions. This also
is used to introduce new language-like "features", and is included in every
other header.

## fsys.hpp

This may have local extensions to C++ filesystem.hpp for portable operations
The most interesting are functional parsing of generic text files and directory
trees in a manner much like Ruby closures offer.

## locking.hpp

This offers a small but interesting subset of ModernCLI classes that focus on
the idea that locking is access. This embodies combining a unique or shared
mutex with a private object that can only be accessed thru scope guards when a
lock is acquired. This offers consistent access behavior for lock protected
objects. The locking containers use protected data so that you can derive a
class with member functions that can directly access or modify atomic fields or
counters inside the data object without locking.

## output.hpp

Some output helpers I commonly use as well providing simple logging support.

## networks.hpp

Animates the network interfaces list into a stl compatible container and
provides helper functions for finding what network interface an address
belongs to or to find interfaces for binding to subnets.

## pipeline.hpp

This is used to move objects between producer and consumer threads much like go
channels might work. Pipeline is optimized to use move semantics when possible
and to clear objects from the pipeline when they are removed. The pipeline is
constructed in a sized template so that it can be in static or stack space
without any heap allocations.

When pipelines are made for pointers, pipeline assumes the objects in the
pipeline are made from "new". If objects have to be dropped, they may be
deleted.

## print.hpp

Performs print formatting, including output to existing streams such as output
logging. Includes helper functions for other busuto types. Because this header
has to include other types, it may include a large number of headers.

## process.hpp

Manage and spawn child processes from C++. Manage file sessions with pipes,
essentially like popen offers.

## resolver.hpp

This provides an asynchronous network resolver that uses futures with support
for reverse address lookup. It also provides a stl compatible container for
examining network resolver addresses.

## safe.hpp

Safe memory operations and confined memory input/output stream buffers that can
be used to apply C++ stream operations directly on a block of memory. This
allows memory blocks, such as from UDP messages, to be manipulated in a very
manner similar to how streams.hpp may be used to parse and produce TCP
session content with full support for C++ stream operators and formatting.

Safe slots is meant to ba a pure slot object for (lockable) data structures in
memory. It is not meant to be used as a value store like std::array. Offset
indexing allows the index to be a "meaningful" value rather than a 0 offset
index. This lets code use an index of 0 to indicate an invalid index.

Safe alsoprovides memory safe C char ptr operations. There is also a fixed
sized (templated) stringbuf class that lets you create a string type that is
very friendly to integration with C strings. A new memory safe version of
getline is provided to read lines into a stringbuf or a fixed char array.

## scan.hpp

Common functions to parse and extract fields like numbers and quoted strings
from a string view. As a scan function extracts, it also updates the view. The
low level scan functions will eventually be driven from a scan template class
that has a format string much like format. Other upper level utility functions
will also be provided.

## services.hpp

Support for writing service applications in C++. This includes a timer system
as well as support for task oriented tread pools and queue based task event
dispatch.

## socket.hpp

Generic basic header to wrap platform portable access to address storage for
low level BSD sockets api.

## streams.hpp

This offers enhanced, performant full duplex system streaming tied to and
optimized for socket descriptor based I/O. Among the enhancements over the C++
streambuf and iostream system is support for high performance zero-copy buffer
read operations. Stream buffering is done with sized templates so that they
can appear in stack space without heap allocations.

## strings.hpp

Generic string utility functions. Many of these are much easier to use and much lighter weight than boost algorithm versions, and are borrowed from moderncli.

## sync.hpp

This introduces scoped guards for common C++17 and C++20 thread synchronization
primitives. I also provide a golang-like wait group. The use cases for golang
wait groups and the specific race conditions they help to resolve apply equally
well to detached C++ threads.

## system.hpp

Just some convenient C++ wrappers around system handles (file descriptors). It
may add some process level functionality eventually, too. It is also meant to
include the hpx init functions and be a basic application main include. The
low level handle system is socket and tty aware, making system streams in
streams.hpp also aware.

## threads.hpp

Convenient base header for threading support in other headers. A common thread
class is used based on std::jthread. As the BSD libraries do not include
jthread, a built-in substitute is offered for those platforms.

## linting

Since this is common code extensive support exists for linting and static
analysis.
