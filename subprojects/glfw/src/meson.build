default_library = get_option('default_library')
if default_library == 'shared'
    set_variable('buildArg', '/D_GLFW_BUILD_DLL=1') 
elif default_library == 'static'
    set_variable('buildArg', '') 
endif

lib = library('glfw',
    'context.c',
    'egl_context.c',
    'init.c',
    'input.c',
    'monitor.c',
    'osmesa_context.c',
    'wgl_context.c',
    'vulkan.c',
    'wgl_context.c',
    'win32_init.c',
    'win32_joystick.c',
    'win32_monitor.c',
    'win32_thread.c',
    'win32_time.c',
    'win32_window.c',
    'window.c',
    version : '3.3.0',
    include_directories : incdir,
    c_args : ['/D_GLFW_WIN32', buildArg])