schema_dir = join_paths('../schema/example')

add_test_setup('valgrind', exe_wrapper: ['valgrind', '-v'])
tests = [
    ['can', ['can.c'], files(join_paths(schema_dir, 'sae-standard.yaml'))],
]
test_include = include_directories('include')
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
