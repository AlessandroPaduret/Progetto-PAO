# Copilot Instructions

## Project Overview

This is a C++ Object-Oriented Programming (OOP) project for the *Programmazione a Oggetti* university course at the University of Padova. It implements an event management system with support for recurrent events, date generators, and scheduling.

## Architecture

The codebase is organized under `include/events/` (headers) and `src/events/` (implementations), split into four layers:

- **core/** – Foundational abstractions: `CommonTypes.h` (type aliases like `TimePoint`, `Duration`, `Clock`), `Schedulable`, `DateGenerator`, `RecurrenceStrategy`, `ItemProvider`, `GroupSchedulable`.
- **domain/** – Main domain classes: `Event`, `RecurrentEvent`, `EventFactory`.
- **generators/** – Concrete `DateGenerator` implementations: `FixedIntervalGenerator`, `YearlyGenerator`, `ExceptionGenerator`, `DateGeneratorDecorator`, `NullGenerator`.
- **providers/** – Concrete `ItemProvider` implementations: `ModificationProvider`, `NullProvider`, `ItemProviderDecorator`.

The convenience header `include/events/events.h` includes everything at once. All code lives in the `events` namespace.

## Build System

The project uses **GNU Make** with a `g++` compiler targeting **C++20**. Docker is provided for a reproducible Qt6 environment.

```bash
# Build the project
make

# Clean build artifacts
make clean

# Format code with clang-format
make format
```

The Makefile auto-discovers all `.cpp` files under `src/` and mirrors the directory structure into `build/`. The test entry point is `tests/test.cpp`, and the final binary is produced at `bin/test.out`.

### Docker Environment

The Docker image (`unipd-oop/qt-env:2025`) provides Ubuntu 24.04 with `g++`, Qt6, `cmake`, `clang-format`, and SQLite. Build and run it per the README instructions.

## Code Conventions

- **Standard**: C++20 (`-std=c++20`).
- **Namespace**: All event-related code is inside the `events` namespace.
- **Header guards**: Use `#ifndef / #define / #endif` include guards; name them `<FILENAME>_H` in uppercase.
- **Includes**: Headers go under `include/`, sources under `src/`. Use paths relative to `include/` (e.g. `#include "events/domain/Event.h"`).
- **Smart pointers**: Prefer `std::unique_ptr` and `std::shared_ptr`; avoid raw owning pointers.
- **Type aliases**: Use the project's type aliases (`TimePoint`, `Duration`, `Clock`, `String`) from `CommonTypes.h` instead of spelling out the full `std::chrono` types.
- **Formatting**: Run `make format` (clang-format) before committing.
- **Compiler flags**: `-Wall -Wextra` are enabled; fix all warnings before committing.

## Testing

Tests live in `tests/test.cpp` and use a `main()` function that exercises the public API. Run the test binary after building:

```bash
make && ./bin/test.out
```

There is no separate unit-test framework; tests print output to stdout. Add new test cases to `tests/test.cpp` and verify the output manually.

## Key Dependencies

- **C++20 standard library** (especially `<chrono>` with calendar types and literals).
- **Qt6** (for the GUI layer, not yet implemented — see TODO in README).
- **SQLite** (`libsqlite3`) for persistence (planned).
- **clang-format** for code style enforcement.
