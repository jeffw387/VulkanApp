project('VulkanApp', 'cpp')

freetype_proj = subproject('freetype')
freetype = freetype_proj.get_variable('freetype_dep')
glfw_proj = subproject('glfw')
glfw = glfw_proj.get_variable('glfw_dep')
stb_proj = subproject('stb')
stb = stb_proj.get_variable('stb_dep')
btree_proj = subproject('btree')
btree = btree_proj.get_variable('btree_dep')
vma_proj = subproject('vma')
vma = vma_proj.get_variable('vma_dep')
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

contentroot = ''.join(['/DCONTENTROOT=','"',meson.source_root(),'/"'])
message(contentroot)
deps = [freetype, stb, glfw, btree, vma, vulkan, glm]
executable('VulkanApp', 
    'main2.cpp',
    'VulkanFunctions.cpp',
    'Allocator.cpp',
    dependencies : deps,
    override_options : 'cpp_std=c++17',
    cpp_args : ['/std:c++latest', 
    '/DVK_USE_PLATFORM_WIN32_KHR', 
    '/DVK_NO_PROTOTYPES',
    contentroot,
    glfwLink,
    '/D_SCL_SECURE_NO_WARNINGS'],
    install : true)