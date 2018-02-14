project('googletest', 'cpp')

inc = include_directories('include')

lib = static_library('gtest',
    'src/gtest-all.cc',
    include_directories : inc,    
    override_options : 'cpp_std=c++17',
    cpp_args : ['/DGTEST_OS_WINDOWS'])

gtest_dep = declare_dependency(include_directories : inc,
link_with : lib)