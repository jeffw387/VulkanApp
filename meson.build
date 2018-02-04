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

contentroot = ''.join(['/DCONTENTROOT=','"',meson.source_root(),'/"'])
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