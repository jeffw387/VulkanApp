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

deps = [freetype, stb, glfw, btree, vma, vulkan, glm]
executable('VulkanApp', 
    'main2.cpp', 
    dependencies : deps,
    override_options : 'cpp_std=c++17',
    cpp_args : ['std:c++latest', '/DVK_USE_PLATFORM_WIN32_KHR', '/DVK_NO_PROTOTYPES', '/DVK_NO_STDINT_H', '/DSTBI_NO_STDIO', '/DGLFW_DLL'],
    link_args : ['/LIBPATH:%VULKAN_SDK%\Lib', '/DGLFW_DLL',
    'vulkan-1.lib', 
    'kernel32.lib',
    'user32.lib',
    'gdi32.lib',
    'winspool.lib',
    'comdlg32.lib',
    'advapi32.lib',
    'shell32.lib',
    'ole32.lib',
    'oleaut32.lib',
    'uuid.lib',
    'odbc32.lib',
    'odbccp32.lib'])