# Summary.
project('yobd', 'c',
    version: '0.1',
    license: 'proprietary',
    default_options: [
        'c_std=c11',
        'buildtype=debug',
        'warning_level=3',
        'werror=true'])

cflags = ['-Wshadow', '-fvisibility=internal']
add_project_arguments(cflags, language: 'c')

pkg = import('pkgconfig')

# Includes.
include = include_directories('include')
install_subdir('include/yobd', install_dir : 'include')

# Config header generation.
conf = configuration_data()
yobd_confdir = join_paths(
    get_option('prefix'),
    get_option('sysconfdir'),
    'yobd')
yobd_schemadir = join_paths(yobd_confdir, 'schema')
conf.set_quoted('CONFIG_YOBD_SCHEMADIR', yobd_schemadir)
configure_file(
    input: 'include/yobd-private/config.h.in',
    output: 'config.h',
    configuration : conf)

# Library.
src = [
    'src/error.c',
    'src/eval.c',
    'src/expr.c',
    'src/parser.c',
    'src/unit.c'
]

thread_dep = dependency('threads')
xlib_dep = dependency('xlib')
yaml_dep = dependency('yaml-0.1')
deps = [
    thread_dep,
    xlib_dep,
    yaml_dep,
]

lib = library(
    'yobd',
    src,
    include_directories: include,
    install: true,
    dependencies: deps,
    version: meson.project_version())
pkg.generate(
    name: 'yobd',
    description: 'A standalone library for schema-driven data translation from CAN to OBD II',
    libraries: [lib],
    version: meson.project_version())
yobd_dep = declare_dependency(link_with: lib, include_directories: include)

# Install schemas.
install_subdir(
    'schema/deploy',
    install_dir: yobd_schemadir,
    strip_directory: true)

# Tests.
add_test_setup('valgrind', exe_wrapper: ['valgrind', '-v'])
tests = [
    ['can', ['test/can.c'], files('schema/deploy/standard-pids.yaml') ],
]
test_include = include_directories('test/include')
test_deps = [yobd_dep] + [xlib_dep]
foreach t : tests
    exe = executable(
        t.get(0),
        t.get(1),
        include_directories: test_include,
        link_with: lib,
        dependencies: test_deps)
    test(t.get(0), exe, args: t.get(2))
endforeach


# Documentation.
run_target('docs', command: 'meson/makedoc')

# Static analysis.
run_target('check', command: 'meson/check')
run_target('clang-tidy', command: 'meson/clang-tidy')
run_target('schema-check', command: 'meson/schema-check')
