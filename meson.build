# Summary.
project('yobd', 'c',
    version: '0.1',
    license: 'proprietary',
    default_options: [
        'c_std=c11',
        'buildtype=debug',
        'warning_level=3',
        'werror=true'])

cflags = [
    '-Wshadow',
    '-Wno-maybe-uninitialized',
    '-fvisibility=internal']
add_project_arguments(cflags, language: 'c')

pkg = import('pkgconfig')

# sysconfdir is special, as when the prefix is /usr, it gets set to /etc. So we
# need to treat this specially so it always gets installed in the right place.
pkgconfig_vars = []
prefix = get_option('prefix')
sysconfdir = get_option('sysconfdir')
if prefix == '/usr'
    base = ''
    pkgconfig_base = ''
else
    base = prefix
    pkgconfig_base = '${prefix}'
endif
pid_dir = join_paths(sysconfdir, 'pids')
yobd_schemadir = join_paths(base, pid_dir)
schemadir_pkgconfig = join_paths(pkgconfig_base, pid_dir)
pkgconfig_vars += ['schemadir=' + schemadir_pkgconfig]

subdir('src')

if get_option('build-tests')
    subdir('test')
endif

if get_option('install-examples')
    install_subdir(
        'schema/example',
        install_dir: yobd_schemadir,
        strip_directory: true)
endif

if get_option('install-tools')
    install_data('scripts/merge-pids', install_dir: get_option('bindir'))
endif

# Documentation.
run_target('docs', command: 'meson/makedoc')

# Static analysis.
run_target('check', command: 'meson/check')
run_target('clang-tidy', command: 'meson/clang-tidy')
run_target('schema-check', command: 'meson/schema-check')
