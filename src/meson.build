# Includes.
include = include_directories('../include')
install_subdir('../include/yobd', install_dir : 'include')

# Config header generation.
conf = configuration_data()
conf.set_quoted('CONFIG_YOBD_PID_DIR', yobd_schemadir)
configure_file(
    input: '../include/yobd-private/config.h.in',
    output: 'config.h',
    configuration : conf)

# Library.
src = [
    'error.c',
    'eval.c',
    'expr.c',
    'parser.c',
    'unit.c'
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
