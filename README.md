# About Busuto

Busuto is a template library for enterprise C++ server development. It will
replaces ModernCLI going forward. Where it differs from ModernCLI is in core
practices and in supporting evolving C++ standards and practices much more
quickly, such as C++20. While ModernCLI and things like Boost all backport many
things for use with C++17, Busuto looks to boost into more current C++ compiler
standards directly.

While both ModernCLI and Busuto are header based C++ libraries, there are
essential differences in how this package is maintained. For example, all
headers use busuto/common.hpp and individual header files are not intended to
be purely stand-alone like they are in ModernCLI. Some stylistic aspects and
practices are also meant to better fit better with Boost libraries. Type traits
and other compiler assisted code generation is used by default and more
commonly than in the current ModernCLI codebase.

From a practical target perspective I assume clang or GCC are the only
compilers explicitly supported. The one major point of departure from ModernCLI
is use of a busuto runtime library. This makes it easier to anchor service
development features such as having a global timer. a global default thread
pool, and a system logger. It also provides a better place to put large blocks
of un-templated functions. On posix we also introduce no runtime library code
that forces injection of additional runtime library dependencies.

Microsoft Windows is not supported as a platform with Busuto, not even with
MingW32. This is largely due to the inability to mix Winsock and mingw posix
file descriptors and use common logic for both. That requires fixing multiple
i/o buffering layers. MingW32 js supported with libcpr because there was only
one simple place in that codebase that needed fixing for that. There were also
so many features that were disabled to support windows. It's just not worth it.

## Dependencies

Busuto itself depends only on C++20 and a C++20 updated libc++.

## Distributions

Distributions of this package are provided as detached source tarballs made
from a tagged release from our public github repository or by building the dist
target. These stand-alone detached tarballs can be used to make packages for
many GNU/Linux systems, and for BSD ports. They may also be used to build and
install the software directly on a target platform.

## Licensing

Busuto Copyright (C) 2025 David Sugar <tychosoft@gmail.comSug>,

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

NOTE: As primarily a header based library where functional code residing in
headers that are either directly called or instantiated and called by user
applications, it is strongly believed any use of this library constitutes and
effectively produces a derivative or combined work, per the GPL, and this is
intentional. As the sole copyright holder I can also offer other forms of
commercial licensing under different terms.

## Participation

This project is offered as free (as in freedom) software for public use and has
a public project page at https://www.github.com/dyfet/busuto which has an issue
tracker where you can submit public bug reports and a public git repository.
Patches and merge requests may be submitted in the issue tracker or thru email.
Other details about participation may be found in CONTRIBUTING.md.

## Testing

There is a testing program for each header. These run simple tests that will be
expanded to improve code coverage over time. The test programs are the only
build target making this library by itself, and the test programs in this
package work with the cmake ctest framework. They may also be used as simple
examples of how a given header works. There is also a **lint** target that can
be used to verify code changes.

