project('evmod_physics', 'c', 'cpp',
version : '0.1',
default_options : [
  'warning_level=3', 
  'c_std=c11',
])

# Meson modules import
fs = import('fs')
cmake = import('cmake')

bullet_opt_var = cmake.subproject_options()

bullet_opt_var.add_cmake_defines({'BUILD_EXTRAS': false})
bullet_opt_var.add_cmake_defines({'BUILD_CPU_DEMOS': false})
bullet_opt_var.add_cmake_defines({'BUILD_OPENGL3_DEMOS': false})
bullet_opt_var.add_cmake_defines({'BUILD_BULLET2_DEMOS': false})
bullet_opt_var.add_cmake_defines({'BULLET_PHYSICS': true})

bullet_opt_var.add_cmake_defines({'CMAKE_BUILD_TYPE': get_option('buildtype')})
bullet_opt_var.add_cmake_defines({'USE_MSVC_RUNTIME_LIBRARY_DLL': true})
bullet_opt_var.add_cmake_defines({'USE_MSVC_RELEASE_RUNTIME_ALWAYS': false})


bullet3_proj = cmake.subproject('bullet3', options: bullet_opt_var)
bullet_dynamics_dep = bullet3_proj.dependency('BulletDynamics')
bullet_collision_dep = bullet3_proj.dependency('BulletCollision')
linear_math_dep = bullet3_proj.dependency('LinearMath')

cc = meson.get_compiler('c')

mod_src = [
  'src/mod.c',
  'src/cpp/physics.cpp',
]

cmod_args = []
cppmod_args = []

# GCC arg for ms-extensions (specifically, nameless anonymous structs/unions)
if build_machine.system() != 'windows'
  cmod_args += '-fms-extensions'
else
  cppmod_args += ['/Zc:preprocessor']
endif

# Metafiles c_args defs
if fs.is_file('meta/evmod.namespaces')
  cmod_args += '-DEVMOD_NAMESPACES_DEF'
endif
if fs.is_file('meta/evmod.events')
  cmod_args += '-DEVMOD_EVENTS_DEF'
endif
if fs.is_file('meta/evmod.types')
  cmod_args += '-DEVMOD_TYPES_DEF'
endif

mod_incdir = ['..', 'include']

evol_dep = dependency('evol')

dl_dep = cc.find_library('dl', required: false)

mod_deps = [
  evol_dep,
  dl_dep,
  bullet_dynamics_dep,
  bullet_collision_dep,
  linear_math_dep,
]

moduleconfig = get_option('moduleconfig')
configure_file(
  input: moduleconfig,
  command: [
    meson.source_root() + '/subprojects/luajit/src/luajit',
    meson.source_root() + '/subprojects/evol/buildscripts/bin2cstring.lua',
    meson.project_source_root() + '/' + moduleconfig
  ],
  output: moduleconfig + '.h',
  capture: true
)
cmod_args += '-DEVMOD_LUA='+meson.project_build_root()+'/'+moduleconfig+'.h'

module = shared_module(
  'evmod_bullet', mod_src,
  include_directories: mod_incdir,
  dependencies: mod_deps,
  c_args: cmod_args,
  cpp_args: cppmod_args,
)

mod_dep = declare_dependency(
  include_directories: mod_incdir,
)

if meson.version().version_compare('>= 0.54.0')
  meson.override_dependency('evmod_physics', mod_dep)
endif
