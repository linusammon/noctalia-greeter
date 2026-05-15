project('noctalia-greeter', ['c', 'cpp'],
  version: '0.1.0',
  default_options: [
    'cpp_std=c++20',
    'cpp_extensions=false',
    'warning_level=3',
  ],
)

cpp = meson.get_compiler('cpp')
datadir = get_option('prefix') / get_option('datadir') / 'noctalia-greeter'

add_project_arguments('-DNOCTALIA_GREETER_DATADIR="@0@"'.format(datadir), language: 'cpp')

egl_dep            = dependency('egl')
gles2_dep          = dependency('glesv2')
gbm_dep            = dependency('gbm')
libdrm_dep         = dependency('libdrm')
wayland_client_dep = dependency('wayland-client')
wayland_egl_dep    = dependency('wayland-egl')
wayland_protos_dep = dependency('wayland-protocols')
cairo_dep          = dependency('cairo')
pango_dep          = dependency('pango')
pangocairo_dep     = dependency('pangocairo')
pangoft2_dep       = dependency('pangoft2')
librsvg_dep        = dependency('librsvg-2.0', include_type: 'system')
xkbcommon_dep      = dependency('xkbcommon')
pam_dep            = cpp.find_library('pam')
util_dep           = cpp.find_library('util', required: false)

wayland_scanner = find_program('wayland-scanner')
wayland_protos_dir = wayland_protos_dep.get_variable('pkgdatadir')

xdg_shell_h = custom_target('xdg-shell-client-header',
  input: wayland_protos_dir / 'stable/xdg-shell/xdg-shell.xml',
  output: 'xdg-shell-client-protocol.h',
  command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
)
xdg_shell_c = custom_target('xdg-shell-client-source',
  input: wayland_protos_dir / 'stable/xdg-shell/xdg-shell.xml',
  output: 'xdg-shell-client-protocol.c',
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  depends: xdg_shell_h,
)

sources = files(
  'src/auth/pam_authenticator.cpp',
  'src/core/log.cpp',
  'src/core/resource_paths.cpp',
  'src/main.cpp',
  'src/platform/debug_wayland_platform.cpp',
  'src/platform/drm_platform.cpp',
  'src/render/core/shader_program.cpp',
  'src/render/gl_renderer.cpp',
  'src/render/programs/rect_program.cpp',
  'src/render/shader_program.cpp',
  'src/render/text_renderer.cpp',
  'src/render/glyph_registry.cpp',
  'src/session/session_launcher.cpp',
  'src/session/session_registry.cpp',
  'src/system/user_registry.cpp',
  'src/system/vt_manager.cpp',
  'src/ui/controls/input.cpp',
  'src/ui/login_view.cpp',
)

executable('noctalia-greeter',
  [sources, xdg_shell_c],
  include_directories: include_directories('src'),
  dependencies: [
    egl_dep,
    gles2_dep,
    gbm_dep,
    libdrm_dep,
    wayland_client_dep,
    wayland_egl_dep,
    cairo_dep,
    pango_dep,
    pangocairo_dep,
    pangoft2_dep,
    librsvg_dep,
    xkbcommon_dep,
    pam_dep,
    util_dep,
  ],
  install: true,
)

session_registry_test = executable('session-registry-test',
  files(
    'tests/session_registry_test.cpp',
    'src/session/session_registry.cpp',
  ),
  include_directories: include_directories('src'),
)

test('session registry', session_registry_test)

input_control_test = executable('input-control-test',
  files(
    'tests/input_control_test.cpp',
    'src/ui/controls/input.cpp',
  ),
  include_directories: include_directories('src'),
  dependencies: [xkbcommon_dep],
)

test('input control', input_control_test)

install_data('data/pam/noctalia-greeter', install_dir: '/etc/pam.d')
install_subdir('assets/fonts', install_dir: get_option('datadir') / 'noctalia-greeter')
install_data('assets/noctalia.svg', install_dir: get_option('datadir') / 'noctalia-greeter')
