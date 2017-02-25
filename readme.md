# libcx3

An experimental C++17 standard library.

## Summary

libCX3 is an experimental work-in-progress, for use in place of C++'s standard library.

Key features include:

* Completely non-reliant on the C++ stdlib; but does rely on the C stdlib.
* Makes use of some bleeding-edge C++ features (C++17).
* Doesn't use class-polymorphism, exceptions, or RTII.
* C-style API. Object composition and function overloading is used instead of inheritance and method dispatch.
* Targets Clang 3.9, with support for Linux and Windows.
* Very minimal use of macros (with exception of debug.cpp & debug.hpp).
* Compiles warning-free (`-Weverything`, with few exceptions).

libCX3 is experimental, and its design decisions should be understood as such.

## License

Copyright © 2016-2017 Yarin Licht

Licensed under the Apache License, Version 2.0 (the “License”);
you may not use this software except in compliance with the License.
You may obtain a copy of the License at

&nbsp;&nbsp;&nbsp;&nbsp;http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an “AS IS” BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

