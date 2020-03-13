# Includes.
include = include_directories('../include')
install_subdir('../include/yobd', install_dir : 'include')

# Config header generation.
conf = configuration_data()

# sysconfdir is special, as when the prefix is /usr, it gets set to /etc. So we
# need to treat this specially so it always gets installed in the right place.
prefix = get_option('prefix')
sysconfdir = get_option('sysconfdir')
if prefix == '/usr'
    base = sysconfdir
else
    base = join_paths(prefix, sysconfdir)
endif
yobd_confdir = join_paths(base, 'yobd')
yobd_schemadir = join_paths(yobd_confdir, 'pids')

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
