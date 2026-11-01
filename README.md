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

To depend on poivre from another CMake project - vendored as a git submodule, the same way
chaudière/misère/tonnerre/chapeau already vendor it:

```cmake
add_subdirectory(poivre)
target_link_libraries(my_tests PRIVATE poivre)
```

That's it - no `-I` needed; the include directory and C++20 requirement both propagate
automatically. `add_subdirectory()` also skips building poivre's own self-tests by default
(only relevant when poivre itself is the top-level project), so it doesn't add an extra
test binary to your build.

The Makefile isn't going anywhere - both build systems produce the same three sources
(`TestCase.cpp`, `TestSuite.cpp`, `TestRegistry.cpp`) and stay in sync with each other, so use
whichever fits your project.

## Writing a test suite

A test suite is a class that derives from `poivre::TestSuite` and implements `runTests()`.
Each individual test is a member function that starts with the `TEST_CASE` macro, which
names the test (for output) and triggers the suite's `setup()`/`tearDown()` hooks around it.

```cpp
#include "TestSuite.h"
#include "TestMacros.h"

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
c++ -std=c++20 -I/path/to/poivre my_tests.cpp \
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
#include "Runnable.h"
#include "BasicException.h"

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

- `getTempFile()` / `deleteFile(path)` — create/remove a scratch file for tests that need one
- `getNumTestsExecuted()` (assertions evaluated), `getNumFailures()`, `getNumTestCasesRun()` —
  inspect results after `run()`
- `poivre::AutoPointer<T>` — a minimal RAII owning-pointer used internally (e.g. by
  `requireException` to own the `Runnable`)

## A complete example

See [`tests/SelfTests.cpp`](tests/SelfTests.cpp) — poivre's own test suite, which exercises
every assertion type, exception matching, temp files, and the setup/tearDown lifecycle.
