# Summary.
project('yobd', 'c',
    version: '0.1',
    license: 'proprietary',
    default_options: [
        'c_std=c11',
        'buildtype=debug',
        'warning_level=3',
        'werror=true'])

cflags = ['-Wshadow', '-fvisibility=hidden']
add_project_arguments(cflags, language: 'c')

pkg = import('pkgconfig')

# Includes.
include = include_directories('include')
install_subdir('include/yobd', install_dir : 'include')

# Library.
src = [
    'src/eval.c',
    'src/expr.c',
    'src/parser.c'
]

thread_deps = dependency('threads')
deps = [
    thread_deps,
    dependency('yaml-0.1')
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

# Tests.
add_test_setup('valgrind', exe_wrapper: ['valgrind', '-v'])
tests = [
    ['can', ['test/can.c'], files('data/min-pids.yaml') ],
]
foreach t : tests
    exe = executable(
        t.get(0),
        t.get(1),
        include_directories: include,
        link_with: lib,
        dependencies: [thread_deps])
    test(t.get(0), exe, args: t.get(2))
endforeach


# Documentation.
run_target('docs', command: 'meson/makedoc')

# Static analysis.
run_target('check', command: 'meson/check')
run_target('clang-tidy', command: 'meson/clang-tidy')
run_target('schema-check', command: 'meson/schema-check')
