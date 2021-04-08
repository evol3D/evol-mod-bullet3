project('evmod_physics', 'c', 'cpp',
version : '0.1',
default_options : [ 'warning_level=3', 'c_std=c11', ])

src = subproject('evol').get_variable('evolmod_subdir') + '/meson.build'
dst = meson.current_source_dir() + '/import/'
run_command( 'python', meson.source_root() + '/subprojects/evol/buildscripts/copy.py', src, dst)
subdir('import')

cmake = import('cmake')

bullet_opt = cmake.subproject_options()

bullet_opt.add_cmake_defines({'BUILD_EXTRAS': false})
bullet_opt.add_cmake_defines({'BUILD_CPU_DEMOS': false})
bullet_opt.add_cmake_defines({'BUILD_OPENGL3_DEMOS': false})
bullet_opt.add_cmake_defines({'BUILD_BULLET2_DEMOS': false})
bullet_opt.add_cmake_defines({'BULLET_PHYSICS': true})

bullet_opt.add_cmake_defines({'CMAKE_BUILD_TYPE': get_option('buildtype')})
bullet_opt.add_cmake_defines({'USE_MSVC_RUNTIME_LIBRARY_DLL': true})
bullet_opt.add_cmake_defines({'USE_MSVC_RELEASE_RUNTIME_ALWAYS': false})


bullet3_proj = cmake.subproject('bullet3', options: bullet_opt)
bullet_dynamics_dep = bullet3_proj.dependency('BulletDynamics')
bullet_collision_dep = bullet3_proj.dependency('BulletCollision')
linear_math_dep = bullet3_proj.dependency('LinearMath')

mod_src = [
  'src/mod.c',
  'src/cpp/physics.cpp',
]

mod_incdir = [
  '..', 
  'include',
]

mod_deps = [
  evmod_deps,

  bullet_dynamics_dep,
  bullet_collision_dep,
  linear_math_dep,
]

module = shared_module(
  'evmod_bullet', mod_src,
  include_directories: mod_incdir,
  dependencies: mod_deps,
)

mod_dep = declare_dependency(
  include_directories: mod_incdir,
)

meson.override_dependency('evmod_physics', mod_dep)
