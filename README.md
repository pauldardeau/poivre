[![Build](https://github.com/pauldardeau/poivre/actions/workflows/cpp-build.yml/badge.svg)](https://github.com/pauldardeau/poivre/actions/workflows/cpp-build.yml)

# poivre

A small, dependency-free C++ unit testing library.

This project has no dependencies on other projects. The Makefile builds a single shared
library file named `poivre.so`; the CMake build (below) produces a static library by default.
Either way, only three source files are involved: `TestCase.cpp`, `TestSuite.cpp`, and
`TestRegistry.cpp`.

## Building

```bash
make        # builds poivre.so
make test   # builds and runs poivre's own self-tests (tests/SelfTests.cpp)
```

### Building with CMake

A `CMakeLists.txt` is also provided, mainly for consumers who want to pull poivre in as a
proper dependency rather than hand-writing `-I` flags:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure   # poivre's self-tests + subprocess regression tests (tests/regression)
```

With a multi-config generator (Visual Studio, Xcode) add `--config Debug` to the build and
`-C Debug` to `ctest`. No dependencies beyond a C++20 compiler and CMake are needed. The
CI workflow (`.github/workflows/cpp-build.yml`) builds and tests with Ubuntu GCC, Ubuntu Clang,
Windows MSVC and macOS Apple Clang; the Makefile is POSIX-only.

## Using poivre from CMake

Whichever way you get poivre, link the same target. No `-I` flags are needed, and the C++20
requirement propagates automatically:

```cmake
target_link_libraries(my_tests PRIVATE poivre::poivre)
```

poivre builds as a static library by default and has no dependencies beyond the C++ standard
library (it does not link a thread library; link `Threads::Threads` yourself if your tests start
threads).

**1. `add_subdirectory`** - poivre vendored in your tree, for example as a git submodule (this is
how chaudière and chapeau use it):

```cmake
add_subdirectory(poivre)
target_link_libraries(my_tests PRIVATE poivre::poivre)
```

poivre's own self-tests and install rules are only enabled when poivre is the top-level project,
so your build gets no extra test binary and your `install` is unaffected. If poivre is added more
than once (for example by you and by a submodule), the first definition is used, provided it is a
poivre of the same major version. Configuration stops with an explanation if an unrelated target is
already named `poivre`, if another major version of poivre was added first, or if `poivre::poivre`
was already imported with `find_package(poivre)` (use the installed package or the source tree, not
both).

**2. `FetchContent`** - pin an exact revision:

```cmake
include(FetchContent)
FetchContent_Declare(poivre
   GIT_REPOSITORY https://github.com/pauldardeau/poivre.git
   GIT_TAG        <full commit hash>
)
FetchContent_MakeAvailable(poivre)
target_link_libraries(my_tests PRIVATE poivre::poivre)
```

**3. `find_package`** - install once, use anywhere:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build --config Release --prefix /path/to/prefix
```

```cmake
find_package(poivre CONFIG REQUIRED)    # -DCMAKE_PREFIX_PATH=/path/to/prefix
target_link_libraries(my_tests PRIVATE poivre::poivre)
```

The installed package (`lib/cmake/poivre/`) is relocatable: it can be moved to another directory
and found again, and it contains no source or build directory paths. `POIVRE_INSTALL` (on by
default only when poivre is the top-level project) controls whether the install rules exist.

### Header conventions

The public headers live in `include/poivre/`. Include them as:

```cpp
#include <poivre/TestSuite.h>
#include <poivre/TestMacros.h>
```

The historical flat spelling keeps working in a source tree, so existing code needs no changes:

```cpp
#include "TestSuite.h"      // forwarding header at the repository root
```

Both spellings name the same declarations and can be mixed. Compatibility notes:

- The forwarding headers at the repository root exist for source-tree consumers (`add_subdirectory`,
  FetchContent, or `-I<poivre checkout>` in a Makefile, as chaudière and chapeau do). They are **not
  installed**: an installed package provides only `<poivre/...>`.
- The `.cpp` files stay at the repository root, so Makefiles that compile `../poivre/TestCase.cpp`
  etc. keep working.
- The poivre target keeps its original name `poivre`; `poivre::poivre` is an alias to it in a source
  tree and the exported name in an installed package.
- Prefer `<poivre/...>` in new code: the flat names (`Runnable.h`, `BasicException.h`, ...) are generic
  and collide with other libraries' headers when several directories are on the include path.

The Makefile isn't going anywhere - both build systems produce the same three sources
(`TestCase.cpp`, `TestSuite.cpp`, `TestRegistry.cpp`) and stay in sync with each other, so use
whichever fits your project.

## Writing a test suite

A test suite is a class that derives from `poivre::TestSuite` and implements `runTests()`.
Each individual test is a member function that starts with the `TEST_CASE` macro, which
names the test (for output) and triggers the suite's `setup()`/`tearDown()` hooks around it.

```cpp
#include <poivre/TestSuite.h>
#include <poivre/TestMacros.h>

using namespace poivre;

class MathTests : public TestSuite {
public:
   MathTests() : TestSuite("MathTests") {}

protected:
   void runTests() override {
      testAddition();
      testSubtraction();
   }

private:
   void testAddition() {
      TEST_CASE("addition");
      require(2 + 2 == 4, "2 + 2 should equal 4");
   }

   void testSubtraction() {
      TEST_CASE("subtraction");
      require(5 - 3 == 2, "5 - 3 should equal 2");
   }
};

int main() {
   MathTests suite;
   suite.run();
   return suite.exitCode();   // 0 = all passed; 1 = a failure, or nothing was asserted
}
```

Compile it alongside poivre's sources and your headers directory, for example:

```bash
c++ -std=c++20 -I/path/to/poivre/include my_tests.cpp \
   /path/to/poivre/TestCase.cpp /path/to/poivre/TestSuite.cpp \
   -o my_tests
./my_tests
```

(You can link against `poivre.so` instead if you prefer a shared build; either way the
headers are all you need at compile time.)

## Assertions

All of these are methods on `TestSuite`, so call them directly from within a `TEST_CASE`:

| Method | Fails when |
|---|---|
| `require(bool expression, testDesc)` | `expression` is `false` |
| `requireFalse(bool expression, testDesc)` | `expression` is `true` |
| `requireStringEquals(expected, actual, testDesc)` | `expected != actual` |
| `requireNonEmptyString(actual, testDesc)` | `actual` is empty |
| `requireException(exceptionType, Runnable*, testDesc)` | see below |
| `failTest(message)` | always — unconditionally records a failure |

`testDesc` is optional on all of the above and is only used in failure output.

### Testing that code throws

`requireException` expects a `poivre::BasicException` (or a subclass overriding `getType()`)
to be thrown with a type name matching `exceptionType`. Wrap the code under test in a
`poivre::Runnable` — `requireException` takes ownership of it and calls `run()`:

```cpp
#include <poivre/Runnable.h>
#include <poivre/BasicException.h>

class DivideByZero : public Runnable {
public:
   void run() override {
      throwIfDenominatorIsZero(1, 0);  // expected to throw a BasicException
   }
};
```

```cpp
requireException("DivideByZeroException", new DivideByZero(), "dividing by zero should throw");
```

It also fails (with a descriptive message) if the code throws the wrong exception type,
throws a plain `std::exception`, or doesn't throw at all.

## Suite and test lifecycle hooks

Override any of these `protected` virtuals in your suite as needed:

| Method | Runs |
|---|---|
| `suiteSetup()` / `suiteTearDown()` | once, before/after the whole suite |
| `setup()` / `tearDown()` | once per `TEST_CASE`, before/after each test |

## Exit status, exceptions and empty runs

**Return `exitCode()` from `main()`, not a failure count.** A process exit status is truncated
(to 8 bits on POSIX), so returning 256 or 512 failures directly would report success.

| Use | Returns |
|---|---|
| `suite.exitCode()` (classic suite, after `run()`) | `0` if at least one assertion ran and none failed, otherwise `1` |
| `TestRegistry::instance().runAllAndGetExitCode()` (`POIVRE_TEST_CASE` tests) | the same rule over all registered test cases; `1` if none ran |
| `TestRegistry::instance().runAll()` | the exact failure **count** (not an exit status) |

A suite that evaluates no assertions, or a registry with no test cases, does not pass.

An exception that escapes a test is recorded as a failure instead of ending the process
(`std::exception` with its `what()` text, or any other thrown type). This covers `setup()`, the
test body, `tearDown()`, `suiteSetup()` and `suiteTearDown()`:

- `POIVRE_TEST_CASE` tests are isolated from each other: later test cases still run.
- In a classic suite, `runTests()` is your own code, so after a test method throws the rest of
  that `runTests()` call is skipped (later suites still run). If `suiteSetup()` throws,
  `runTests()` is skipped; `suiteTearDown()` always runs.
- If `setup()` throws, that test's body is not run.

The counters are atomic, so assertions may be made from several threads.

## Other utilities

- `getTempFile()` / `deleteFile(path)` — create/remove a scratch file for tests that need one.
  `getTempFile()` creates a new, empty file (exclusively, so it never reuses or overwrites an
  existing one) in the platform's temporary directory (`TMPDIR`/`TMP`/`TEMP`/`TEMPDIR`, else
  `/tmp`, on POSIX; `GetTempPath` on Windows), named `poivre` plus 16 hex digits, and returns
  its path. On failure it prints a diagnostic and returns an empty string.
- `getNumTestsExecuted()` (assertions evaluated), `getNumFailures()`, `getNumTestCasesRun()` —
  inspect results after `run()`
- `poivre::AutoPointer<T>` — a minimal RAII owning-pointer used internally (e.g. by
  `requireException` to own the `Runnable`)

## A complete example

See [`tests/SelfTests.cpp`](tests/SelfTests.cpp) — poivre's own test suite, which exercises
every assertion type, exception matching, temp files, and the setup/tearDown lifecycle.
