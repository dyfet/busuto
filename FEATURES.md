# Features

HiTycho consists of a series of individual C++ 20 header files that typically
are installed under and included from /usr/include/hitycho for use in HPX
applications. Some of these are template headers, but all are meant to be used
directly inline. These currently include:

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

## finalize.hpp

This offers the ability to finalize scopes thru a compiler optimized template
function. While scope\_defer is somewhat analogous to finalize in C# and golang
defer there is also a "scope\_detach" which then executes the finalize in a
separate HPX thread, thereby not blocking the function return itself. This
version of defer feels truer to the ``spirit'' of HPX though obviously you will
want to consider the scope and lifetimes of any captures or arguments used.

## hash.hpp

Thread-safe consistent hash support for cross-platform distributed computing.
Any simple hash function from any supported crypto.hpp backend may be used,
though it defaults to sha256 if none is specified. This currently supports 64
bit big endian consistent hash function and a hash ring buffer for 64 bit
scattered distributed keys that can be used to dynamically insert and remove
distributed hosts. This core header is also consistent with my implimentations
in other languages.

## locking.hpp

This offers a small but interesting subset of ModernCLI classes that focus on
the idea that locking is access. This embodies combining a unique or shared
mutex with a private object that can only be accessed thru scope guards when a
lock is acquired. This offers consistent access behavior for lock protected
objects.

## output.hpp

Some output helpers I commonly use as well as simple logging support.

## print.hpp

Performs print formatting, including output to existing streams such as output
logging. Includes helper functions for other busuto types. Because this header
has to include other types, it may include a large number of headers.

## scan.hpp

Common functions to parse and extract fields like numbers and quoted strings
from a string view. As a scan function extracts, it also updates the view. The
low level scan functions will eventually be driven from a scan template class
that has a format string much like format. Other upper level utility functions
will also be provided.

## socket.hpp

Generic basic header to wrap platform portable access to address storage for
low level BSD sockets api.

## strings.hpp

Generic string utility functions. Many of these are much easier to use and much lighter weight than boost algorithm versions, and are borrowed from moderlcli.

## sync.hpp

This introduces scoped guards for common C++17 HPX task synchronization classes
and adds some special wrapper versions for sempahores. The golang-like
ModernCLI wait group is also provided for HPX threads. The use cases for golang
waitgroups and the specific race conditions they help to resolve apply equally
well to detached HPX threads.

## system.hpp

Just some convenient C++ wrappers around system handles (file descriptors). It
may add some process level functionality eventually, too. It is also meant to
include the hpx init functions and be a basic application main include.

## threads.hpp

Convenent base header for threading support in other headers.

## timer.hpp

This is a timer system that takes advantage of the ability of HPX to spawn off
thousands of threads cheaply. Rather than using an execute queue of lambdas on
a single OS timer thread each timer has it's own detached HPX thread instance
for both one shot and periodic tasks. These are executed as functional
expressions thru templating so the compiler can also optimize the call site.

## linting

Since this is common code extensive support exists for linting and static
analysis.
