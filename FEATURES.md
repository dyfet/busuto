# Features

HiTycho consists of a series of individual C++ 20 header files that typically
are installed under and included from /usr/include/hitycho for use in HPX
applications. Some of these are template headers, and some require linking with
the busuto library this package builds. These currently include:

PLEASE NOTE: All crypto related features are being moved into a seperate
dedicated header-only support library for Busuto, Himitsu. This was to make it
easier to do a busuto release early while adding TLS support to the crypto
layer. Since the crypto support is header-only and is based on different
backends, it also had a rather different architecture than the rest of Busuto.
Finally, one might choose to adopt different C++ crypto libs for writing
applications with Busuto.

## atomic.hpp

Atomic types and lockfree data structures. This includes lockfree stack,
buffer, and unordered dictionary implimentations which are something like C#
ConcurrentStack, ConcurrentDictionary, and ConcurrentQueue. It also includes
an implimentation of atomic\_ref that should be similar to the C++20 one.

## binary.hpp

This is the generic portable convertable flexible binary data array object
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
objects.

## output.hpp

Some output helpers I commonly use as well providing simple logging support.

## networks.hpp

Animates the network interfaces list into a stl compatible container and
provides helper functions for finding what network interface an address
belongs to or to find interfaces for binding to subnets.

## print.hpp

Performs print formatting, including output to existing streams such as output
logging. Includes helper functions for other busuto types. Because this header
has to include other types, it may include a large number of headers.

## process.hpp

Manage and spawn child processes from C++. Manage file sessions with pipes,
essentially like popen offers.

## resolver.hpp

This provides an asychronous network resolver that uses futures with support
for reverse address lookup. It also provides a stl compatible container for
examining network resolver addresses.

## safe.hpp

Safe memory operations and confined memory input/output stream buffers that can
be used to apply C++ stream operations directly on a block of memory. This
allows memory blocks, such as from UDP messages, to be manipulated in a very
manner similar to how streams.hpp may be used to parse and produce TCP session
content with full support for C++ stream operators and formatting.

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
read operations.

## strings.hpp

Generic string utility functions. Many of these are much easier to use and much lighter weight than boost algorithm versions, and are borrowed from moderlcli.

## sync.hpp

This introduces scoped guards for common C++17 and C++20 thread synchronization
primitives. I also provide a golang-like wait group and a pipeline template
that is like a very simple go channel. The use cases for golang waitgroups and
the specific race conditions they help to resolve apply equally well to
detached C++ threads.

## system.hpp

Just some convenient C++ wrappers around system handles (file descriptors). It
may add some process level functionality eventually, too. It is also meant to
include the hpx init functions and be a basic application main include. The
low level handle system is socket and tty aware, making system streams in
streams.hpp also aware.

## threads.hpp

Convenent base header for threading support in other headers. A common thread
class is used based on std::jthread. As the BSD libraries do not include
jthread, a built-in substitute is offered for those platforms.

## linting

Since this is common code extensive support exists for linting and static
analysis.
