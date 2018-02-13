project('VulkanApp', 'cpp')

freetype_proj = subproject('freetype')
freetype = freetype_proj.get_variable('freetype_dep')
glfw_proj = subproject('glfw')
glfw = glfw_proj.get_variable('glfw_dep')
stb_proj = subproject('stb')
stb = stb_proj.get_variable('stb_dep')
btree_proj = subproject('btree')
btree = btree_proj.get_variable('btree_dep')
vulkan_proj = subproject('vulkan')
vulkan = vulkan_proj.get_variable('vulkan_dep')
glm_proj = subproject('glm')
glm = glm_proj.get_variable('glm_dep')

libtype = get_option('default_library')
if  libtype == 'static'
    set_variable('glfwLink', '')
elif libtype == 'shared'
    set_variable('glfwLink', '/DGLFW_DLL')
endif

buildtype = get_option('buildtype')
if (buildtype == 'release') or (buildtype == 'debugoptimized')
    set_variable('Validation', '/DNO_VALIDATION')
else
    set_variable('Validation', '')
endif

contentroot = '/DCONTENTROOT=' + '"' + meson.source_root() + '\"'
message(contentroot)
message(buildtype)
deps = [freetype, stb, glfw, btree, vulkan, glm]
executable('VulkanApp', 
    'main2.cpp',
    'VulkanFunctions.cpp',
    'Allocator.cpp',
    dependencies : deps,
    override_options : 'cpp_std=c++17',
    cpp_args : ['/std:c++latest', 
    '/DVK_USE_PLATFORM_WIN32_KHR', 
    '/DVK_NO_PROTOTYPES',
    Validation,
    contentroot,
    glfwLink,
    '/D_SCL_SECURE_NO_WARNINGS'],
    install : true)

if (buildtype == 'debug')
    set_variable('gtestlibname', 'gtestd')
    set_variable('gtestdir', 'C:/googletest/debug/')
elif (buildtype == 'release')
    set_variable('gtestlibname', 'gtest')
    set_variable('gtestdir', 'C:/googletest/release/')
endif

cc = meson.get_compiler('cpp')
gtest_inc = include_directories('C:/Libraries/Source/googletest/googletest/include')
gtest_dep = declare_dependency(
    dependencies : cc.find_library(gtestlibname, dirs: gtestdir), 
    include_directories : gtest_inc)

circ_queue_test = executable('CircularQueueTest',
    'test/CircularQueueTest.cpp',
    cpp_args : ['/std:c++latest'],
    dependencies : [gtest_dep])

test('gtest circ_queue', circ_queue_test)

quadtree_test = executable('QuadTreeTest',
    'test/QuadTreeTest.cpp',
    cpp_args : ['/std:c++latest'],
    dependencies : [gtest_dep])

test('gtest quadtree', quadtree_test)