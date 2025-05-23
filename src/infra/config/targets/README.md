# Targets definitions

This directory contains target definitions used for configuring the targets and
tests that will be executed by builders in the chromium project that use the
chromium_tests recipe module for executing tests. This includes most builders
that build and run tests. Builders running PRESUBMIT.py tests and builders
running fuzz tests are not included.

## Files

* [basic_suites.star](./basic_suites.star) - This contains definitions of basic
  suites, which specify tests to include as well as optionally specify various
  details for those tests such as which test target to actually build and run,
  additional args that should be used or default swarming details. These suites
  can be referenced by compound suites in compound_suites.star, matrix compound
  suites in matrix_compound_suites.star or set as a test suite for a builder in
  [waterfalls.pyl][waterfalls.pyl].
* [binaries.star](./binaries.star) - This contains declarations with binary
  mappings used when declaring tests. The declarations map the GN target name to
  the ninja label that needs to be built to produce the actual executable for
  the test. Additional details can be specified as part of the declaration that
  will be used as part of the process of generating target spec files. Tests
  defined using targets.tests.gtest_test and target.tests.isolated_script_test
  all require a binary to be defined, which is set via the binary field. If the
  binary field is not set, then there must be a binary declared that has the
  same name as the test. Tests defined using targets.tests.junit_test implicitly
  define a binary that results in an entry in the generated gn_isolate_map.pyl
  file.

  Available binary types are:
  * console_test_launcher
  * generated_script
  * script
  * windowed_test_launcher
* [bundles.star](./bundles.star) - This contains declarations of bundles, which
  are groupings of tests and compile targets that can be referenced by builders
  that set their tests in starlark. These bundles cannot be referenced by any
  legacy suites or by builders setting their tests in
  [waterfalls.pyl][waterfalls.pyl].
* [compile_targets.star](./compile_targets.star) - This contains definition of
  compile-only targets. These targets can only be built by a builder and have no
  provision for execution. These targets can be included in the
  additional_compile_targets field for a builder in
  [waterfalls.pyl][waterfalls.pyl].
* [compound_suites.star](./compound_suites.star) - This contains definitions of
  compound suites, which specify basic suites to collect into a larger suite.
  These suites can set as a test suite for a builder in
  [waterfalls.pyl][waterfalls.pyl].
* [matrix_compound_suites.star](./matrix_compound_suites.star) - This contains
  definitions of matrix compound suites, which specify basic suites to collect
  into a larger suite, optionally applying mixins to all the tests of a basic
  suite and/or expanding tests in a basic suite with one or more variants. For
  each suite where variants are specified, each test in the suite will generate
  a test spec for each variant with the variant applying modifications to the
  generated test spec. These suites can be set as a test suite for a builder in
  [waterfalls.pyl][waterfalls.pyl].
* [mixins.star](./mixins.star) - This contains definitions of mixins that can be
  used to modify the generated test specs. These mixins can be specified when
  defining the details of a test in a basic suite to modify that test, when
  configuring a basic suite in a matrix compound suite to modify all tests in
  that expansion of the basic suite or on a builder in
  [waterfalls.pyl][waterfalls.pyl] to modify all tests for the builder.
* [tests.star](./tests.star) - This contains definitions of test targets. These
  targets can be referenced in basic suites. Additionally, they can contain
  details such as args or mixins. These details will be transferred to the
  generated test_suites.pyl since //testing/buildbot doesn't use separate target
  declarations for tests.

  Available test types are:
  * gtest_test
  * gpu_telemetry_test
  * isolated_script_test
  * junit_test
  * script_test
* [variants.star](./variants.star) - This contains definitions of variants,
  which allow for expanding a single test definition into multiple generated
  test specs and apply additional modifications to the generated test specs.
  They can be specified when configuring a basic suite in a matrix compound
  suite to expand all of the tests in the basic suite with each of the specified
  variants.

## Tests in starlark

Currently, a migration is in process to enable tests for a builder to be
configured as part of the builder definition itself. In order to avoid having to
manually sync definitions in two locations/systems, some of the .pyl that used
to be hand-written files in //testing/buildbot are now generated from the
starlark definitions. Due to intentional design decisions in lucicfg, the files
generated from the //infra/config starlark can't be located outside of
//infra/config/generated. //testing/buildbot/generate_buildbot_json.py has been
updated to read gn_isolate_map.pyl, mixins.pyl, test_suites.pyl and variants.pyl
from //infra/config/generated/testing.

Updating these .pyl files requires the following process:

1. Modify starlark files

    | .pyl file | starlark file to edit |
    |-----------|-----------------------|
    |gn_isolate_map.pyl|binaries.star (or tests.star for test defined using targets.test.junit_test)|
    |mixins.pyl|mixin.star|
    |variants.pyl|variants.star|
    |test_suites.pyl (basic_suites entries)|basic_suites.star (or tests.star/binaries.star or for values that apply to all instances of a test/binary)|
    |test_suites.pyl (compound_suites entries)|compound_suites.star|
    |test_suites.pyl (matrix_compound_suites entries)|matrix_compound_suites.star|

1. Regenerate the config by running [main.star](/infra/config/main.star)

    `lucicfg infra/config/main.star`

    On mac or linux, you can just do: `infra/config/main.star`

Then you can make any edits you wish to the hand-written .pyl files in
//testing/buildbot ([waterfalls.pyl][waterfalls.pyl] and
[test_suite_exceptions.pyl](/testing/buildbot/test_suite_exceptions.pyl)) and
run [generate_buildbot_json.py](/testing/buildbot/generate_buildbot_json.py) to
generate the targets spec files.

[waterfalls.pyl]: /testing/buildbot/waterfalls.pyl

Due to angle using mixins.pyl via a subtree repo that exports
//testing/buildbot, if mixins.pyl was modified by the above steps, it's
necessary to sync that file to //testing/buildbot.

1. Copy mixins.pyl from //infra/config/generated/testing to
   //testing/buildbot by running
   [sync-pyl-files.py](/infra/config/scripts/sync-pyl-files.py)

    `infra/config/scripts/sync-pyl-files.py`

### Setting tests for a builder in starlark

It is now possible to specify test in starlark for builders in limited
conditions:

* No legacy matrix compound suites can be referenced
* Cannot rely on any fields being set on the builder's waterfall in
  waterfalls.pyl except mixins
* Cannot rely on any fields besides additional_compile_targets, test_suites,
  mixins, os_type and use_swarming being set for the builder in waterfalls.pyl
  (fields that are present in targets.mixin can be specified by specifying an
  in-place mixin in the mixins field instead)
  * Under test_suites, test can only be specified for the scripts,
    gtest_tests and isolated_scripts keys
    * Only the following fields can be set on gtests & isolated scripts either
      directly or via mixins:
      * args
      * android_args (expanding these isn't supported yet, so this won't take
        effect, but won't be rejected)
      * android_swarming (expanding these isn't supported yet, so this won't
        take effect, but won't be rejected)
      * ci_only
      * isolate_profile_data
      * merge
      * precommit_args
      * resultdb
      * swarming
      * test (only allowed in declaration of gtest itself)
      * use_isolated_script_api
* Cannot use variants

These conditions will be removed as more support is implemented.

To set tests for a builder in starlark, the builder must be setting the
builder_spec argument. If that is the case, simply set the targets field of a
builder to the name of a bundle or a list of bundle names to include. Supported
target types create a bundle with the same name as the target, so single targets
can be referenced wherever a bundle is expected.

```starlark
ci.builder(
  name = "foo-builder",
  builder_spec = ...,
  # Configures the builder to build all
  targets = targets.bundle(
      additional_compile_targets = "all",
  ),
)

ci.builder(
  name = "bar-builder",
  builder_spec = ...,
  # Configures the builder to build all and run any tests and build any compile
  # targets specified by the public_build_scripts bundle
  targets = targets.bundle(
      additional_compile_targets = "all",
      targets = "public_build_scripts",
  ),
)
```

Fields that would be specified on the waterfall can instead be set on
targets.builder_defaults, which will apply to all builders defined in the file.

```starlark
targets.builder_defaults.set(
  mixins = [
    "chromium-tester-service-account",
    "linux-jammy",
  ],
)
```

This will cause json files containing the targets specs to be written out in the
per-builder generated directory. Try builders that mirror other builders do not
need to set the targets field (and it is an error to do so). The mirroring
relationship indicates that it should use the targets configured for the
builders that it mirrors, so mirroring try builders will have their own json
files generated containing the logical union of the contents of the mirrored
builders. This makes it more clear what builders are affected by modifications
to a CI builder's targets.
