# Run Tests on Fuchsia with CFv2 Test Scripts

[TOC]

A new version of scripts for testing on Fuchsia is being developed
[here](../../build/fuchsia/test/) and the plan is to migrate all use cases
to these scripts by the end of Q322. The new scripts currently support:

## Run CFv2 gtest binaries on Fuchsia

To build Fuchsia gtest binaries follow
[build instructions](build_instructions.md).

### Run on a transient emulated device

The test script brings up an emulator, runs the tests on it, and
shuts the emulator down when finished.
```bash
$ ./build/fuchsia/test/run_test.py [TEST_BINARY] -C [OUTPUT_DIR]
```

### Run on a persistent emulated device from the Chromium tree

You can start a persistent emulator from the Chromium tree by running this
command:

```bash
$ ./build/fuchsia/test/start_emulator.py
```

Note that running this command for the first time will download a Fuchsia
product bundle and will take around a minute. Part of the output should be:

```
Logging to \
 "$HOME/.local/share/Fuchsia/ffx/emu/instances/[EMULATOR_NAME]/emulator.log"
```

Record the [EMULATOR_NAME] value as it will be used in subsequent commands.

#### E2E Testing Script

Once the emulator is running, you can run tests on this emulator instance by
adding the command line arguments indicated above:

```bash
$ ./build/fuchsia/test/run_test.py [TEST_BINARY] -C [OUTPUT_DIR] -d \
  --target-id [EMULATOR_NAME]
```

#### Step by step approach

Alternatively, testing can be done step by step. The following steps are
equivalent to the [E2E testing script](#e2e-testing-script).

Before starting the steps, it is recommended to set the device that will be used
as the default device that `ffx` uses:

```bash
$ ./third_party/fuchsia_sdk/sdk/tools/x64/ffx target default set [EMULATOR_NAME]
```

If the device is not set as default, all the steps other than package publishing
will require an extra `--target-id [EMULATOR_NAME]` flag.

##### Publish packages

Before devices can run the packages built, a directory needs to be initialized
to serve as a package repository and the packages must be published to it:

```bash
$ ./build/fuchsia/test/publish_package.py -C [OUTPUT_DIR] --repo [REPO_DIR] \
  --packages [TEST_BINARY]
```

##### Package installation

The packages need to be installed on the device:

```bash
$ ./build/fuchsia/test/serve_repo.py start --serve-repo [REPO_DIR] \
```

##### Stream system logs

System logs can be obtained via:

```bash
$ ./build/fuchsia/test/log_manager.py start --packages [TEST_BINARY] \
  -C [OUTPUT_DIR]
```

Specifying the test binary and the output directory allows the logs to be
properly symbolized.

##### Run test package

```bash
$ ./build/fuchsia/test/run_executable_test.py --test-name [TEST_BINARY] \
```
