# yobd
yobd is a standalone library for schema-driven data translation from CAN to OBD
II.  The reason this is necessary is that the OBD II protocol is highly
bitpacked, so each message is interpreted a little bit differently. In addition,
only a subset of the possible OBD II data (PIDs) are publicly available, so each
car make/model can have other PIDs, each intepreted differently. This calls for
a generalized schema for describing such messages, and a generalized runtime for
evaluating such messages and passing them on in easily consumable form (e.g.
JSON or similar hierarchical, string-based format).  Without such a schema, one
needs to either hardcode or do code generation (such as what `OpenXC` does),
both of which have significant issues.

There are a few parts to YoloBD:

- A YAML schema for describing OBD II PIDs. YAML is chosen because it is easy
  for humans to write and read.
- A C parser for converting the YAML schema into an in-memory PID representation
  that can be efficiently evaluated at runtime.
- A C interpreter that can take an in-memory PID representation and produce
  easily-consumable, hierarchical data for applications. Notably, this
  intepreter doesn't know anything about schemas; it just uses the in-memory
  representation.
- A Python schema validator to check a YAML schema at compile-time. This parser
  is used as part of the build but is never deployed. The idea is that the
  runtime components (the parser and inteprereter) can safely assume that all
  schemas it sees are valid. Using this tool, we can massively simplify the C
  parser (by skipping lots of error checking) and catch mistakes at compile-time
  rather than at runtime.

## Build

### Prerequisites
- meson: `pip3 install meson`

- ninja: `pip3 install ninja`

- (optional, for developers) `clang-tidy`. This is used for static analysis and
  is thus a build but not runtime requirement.

- (optional, for developers) Python requirements, as documented in
  `dev-requirements.txt`. These are some build-time tools that are not required
  for runtime. They are used for sanity-checking OBD II schemas. You can install
  them from your distro or via `pip3 install -r dev-requirements.txt`.

### Custom requirements building

Note that, if any of your build prerequisites do not come from standard distro
packaging, you will need also need to tweak the following env vars:

- `PKG_CONFIG_PATH` needs to be set only when you run `meson` and doesn't matter
  after that. It should be set to the directory containing the `.pc` files used
  by the prerequisite you built.
- `LD_LIBRARY_PATH` needs to be set whenever you actually load the YoloBD
  library, such as when you run the unit tests with `ninja test`. It should be
  set to the directory containing the built prerequisite libraries.

### Debian unstable

To get your build requirements, you just need to run:

```
sudo apt-get -y install meson ninja-build
```

### Fedora 24 and 25 packages

To get your build requirements, you just need to run:

```
sudo dnf -y install meson ninja-build
```

Note that on fedora you will substitute the `ninja-build` command instead of
the `ninja` command for the various build commands on this page.

## Instructions

### First time builds

```
mkdir build
cd build
meson ..
ninja
```

### Rebuilding

To rebuild at any time, you just need to rerun the last `ninja` command:

```
ninja
```

You can run this command from any directory you like. For instance, to rebuild
from the top level directory, use the ninja `-C` option to point ninja at the
build directory:

```
ninja -C build
```

Also, there is nothing special about the directory name `build`. If you prefer a
different name than `build`, this is not a problem, and you can have different
build directories with different configurations; meson and ninja don't care.

### Compiling with clang instead of gcc

If you want to use clang, it's the usual meson methodology:

```
mkdir build.clang
cd !$
CC=clang meson ..
ninja
```

### Running tests
Do this to run unit tests:
```
ninja test
```
or
```
mesontest --setup valgrind
# ninja test actually calls mesontest
```

Before checking in, you should run:
```
ninja check
```

Which runs unit tests, does static analysis, and anything else that is deemed
"good hygiene" prior to checking in. This list may change over time, but the
`check` target should remain valid.

### Static analysis
Static analysis uses `clang-tidy` and can be run with:
```
ninja clang-tidy
```

Note that you will need to install the `clang-tidy` tool, either via distro or
some other method.

### Generating documentation
Code documentation is handled by `doxygen` and can be built with:
```
ninja docs
```

# Documentation

See the [doc](doc) folder for more documentation.
