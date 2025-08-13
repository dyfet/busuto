# Features

HiTycho consists of a series of individual C++ 20 header files that typically
are installed under and included from /usr/include/hitycho for use in HPX
applications. Some of these are template headers, and some require linking with
the busuto library this package builds. These currently include:

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

## crypto.hpp

This is meant to give access to a subset of commonly used low level crypto
functions that are often minimally required for server applications. It is not
meant to offer tls or certificate management, but rather lower level things
like random number and hash digest generation, and other core features that
likely are widely needed in service application development.

Busuto crypt sits on top of one of several back-end crypto libraries offering a
common consistent api for core features that is adapted for use with
byte_\array. On Apple it likely will become adapted to use CommonCrypto, for
example, and is meant to serve a similar range of needs. The crypto library is
configured in source thru crypto.hpp to prevent runtime linking issues, and the
selection of which backend will be used can be selected at compile time.

The exact range of features offered may depend on the backend being used, and
while they all overlap on core features with a consistent api, different
backends may may expose unique capabilities that may apply to a specific
subclass of applications. This is especially true of the libsodium backend.
The wolfssl crypto backend seems ideal for more embedded service development.

## digests.hpp

Enanced stream oriented digest support. This makes it possible create and
compute digests from arbitrary input data by pushing content into a stream
buffer that computes a running digest using crypto.hpp supported hashing
algorithms. This provides a very simple and natural C++ api for computing
digests for arbitrary data of arbitrary size.

## fsys.hpp

This may have local extensions to C++ filesystem.hpp for portable operations
The most interesting are functional parsing of generic text files and directory
trees in a manner much like Ruby closures offer.

## hash.hpp

Thread-safe consistent hash support for cross-platform distributed computing.
Any simple hash function from any supported crypto.hpp backend may be used,
though it defaults to sha256 if none is specified. This currently supports 64
bit big endian consistent hash function and a hash ring buffer for 64 bit
scattered distributed keys that can be used to dynamically insert and remove
distributed hosts. This core header is also consistent with my implimentations
in other languages.

## legacy.hpp

Support for legacy crypto algorithms, many of which are being deprecated and
removed from modern crypto libraries. Some of these are algoriths still
actively used on existing devices in the field which may not have upgragable
firmware.

## locking.hpp

This offers a small but interesting subset of ModernCLI classes that focus on
the idea that locking is access. This embodies combining a unique or shared
mutex with a private object that can only be accessed thru scope guards when a
lock is acquired. This offers consistent access behavior for lock protected
objects.

## output.hpp

Some output helpers I commonly use as well providing simple logging support.

## print.hpp

Performs print formatting, including output to existing streams such as output
logging. Includes helper functions for other busuto types. Because this header
has to include other types, it may include a large number of headers.

## process.hpp

Manage and spawn child processes from C++. Manage file sessions with pipes,
essentially like popen offers.

## resolver.hpp

This provides an asychronous network resolver that uses futures with support
for reverse address lookup, and a service to manage a list of active
interfaces. The latter can be used to find the interface and subnet of a socket
connection or to keep track of interfaces that may go up or down. It is often
used to find interfaces for joining multicast sessions and binding sockets
as well.

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
primitives. I also provide a golang-like wait group. The use cases for golang
waitgroups and the specific race conditions they help to resolve apply equally
well to detached C++ threads.

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
