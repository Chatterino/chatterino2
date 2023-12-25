# Test and Benchmark

Chatterino includes a set of unit tests and benchmarks. These can be built using CMake by adding the `-DBUILD_TESTS=On` and `-DBUILD_BENCHMARKS=On` flags respectively.

## Adding your own test

1. Create a new file for the file you're adding tests for. If you're creating tests for `src/providers/emoji/Emojis.cpp`, create `tests/src/Emojis.cpp`.
2. Add the newly created file to `tests/CMakeLists.txt` in the `test_SOURCES` variable (see the comment near it)

See `tests/src/Emojis.cpp` for simple tests you can base your tests off of.

Read up on http://google.github.io/googletest/primer.html to figure out how GoogleTest works.

## Building and running tests

```sh
mkdir build-tests
cd build-tests
cmake -DBUILD_TESTS=On ..
make
./bin/chatterino-test
```

### Example output

```
[==========] Running 26 tests from 8 test suites.
[----------] Global test environment set-up.
[----------] 2 tests from AccessGuardLocker
[ RUN      ] AccessGuardLocker.NonConcurrentUsage
[       OK ] AccessGuardLocker.NonConcurrentUsage (0 ms)
[ RUN      ] AccessGuardLocker.ConcurrentUsage
[       OK ] AccessGuardLocker.ConcurrentUsage (686 ms)
[----------] 2 tests from AccessGuardLocker (686 ms total)

[----------] 4 tests from NetworkCommon
[ RUN      ] NetworkCommon.parseHeaderList1
[       OK ] NetworkCommon.parseHeaderList1 (0 ms)
[ RUN      ] NetworkCommon.parseHeaderListTrimmed
[       OK ] NetworkCommon.parseHeaderListTrimmed (0 ms)
[ RUN      ] NetworkCommon.parseHeaderListColonInValue
...
[ RUN      ] TwitchAccount.NotEnoughForMoreThanOneBatch
[       OK ] TwitchAccount.NotEnoughForMoreThanOneBatch (0 ms)
[ RUN      ] TwitchAccount.BatchThreeParts
[       OK ] TwitchAccount.BatchThreeParts (0 ms)
[----------] 3 tests from TwitchAccount (2 ms total)

[----------] Global test environment tear-down
[==========] 26 tests from 8 test suites ran. (10297 ms total)
[  PASSED  ] 26 tests.
```

## Adding your own benchmark

1. Create a new file for the file you're adding benchmark for. If you're creating benchmarks for `src/providers/emoji/Emojis.cpp`, create `benchmarks/src/Emojis.cpp`.
2. Add the newly created file to `benchmarks/CMakeLists.txt` in the `benchmark_SOURCES` variable (see the comment near it)

See `benchmarks/src/Emojis.cpp` for simple benchmark you can base your benchmarks off of.

## Building and running benchmarks

```sh
mkdir build-benchmarks
cd build-benchmarks
cmake -DBUILD_BENCHMARKS=On ..
make
./bin/chatterino-benchmark
```

### Example output

```
2021-07-18T13:12:11+02:00
Running ./bin/chatterino-benchmark
Run on (12 X 4000 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 15360 KiB (x1)
Load Average: 2.86, 3.08, 3.51
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
--------------------------------------------------------------
Benchmark                    Time             CPU   Iterations
--------------------------------------------------------------
BM_ShortcodeParsing       2394 ns         2389 ns       278933
```
