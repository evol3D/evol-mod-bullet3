project('evmod_physics', 'c', 'cpp',
version : '0.1',
default_options : [ 'warning_level=3', 'c_std=c11', ])

src = subproject('evol').get_variable('evolmod_subdir') + '/meson.build'
dst = meson.current_source_dir() + '/import/'
run_command( 'python3', meson.source_root() + '/subprojects/evol/buildscripts/copy.py', src, dst)
subdir('import')

cc = meson.get_compiler('c')
cc_id = cc.get_id()

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
bullet_opt.add_cmake_defines({'CMAKE_POSITION_INDEPENDENT_CODE': true})
if cc_id == 'gcc'
  bullet_opt.add_cmake_defines({'CMAKE_CXX_FLAGS': '-fno-gnu-unique'})
endif

bullet3_proj = cmake.subproject('bullet3', options: bullet_opt)
bullet_dynamics_dep = bullet3_proj.dependency('BulletDynamics')
bullet_collision_dep = bullet3_proj.dependency('BulletCollision')
linear_math_dep = bullet3_proj.dependency('LinearMath')

mod_src = [
  'src/mod.c',

  'src/cpp/physics.cpp',
  'src/cpp/EvMotionState.cpp',
  'src/cpp/visual-dbg/BulletDbg.cpp',
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

mod_cpp_args = []
if cc_id == 'gcc'
  mod_cpp_args += ['--no-gnu-unique']
endif

module = shared_module(
  'bullet', mod_src,
  include_directories: mod_incdir,
  dependencies: mod_deps,
  cpp_args: mod_cpp_args,
  name_prefix: evmod_name_prefix,
  name_suffix: evmod_name_suffix,
)

mod_dep = declare_dependency(
  include_directories: mod_incdir,
)

meson.override_dependency('evmod_physics', mod_dep)

configure_file(input: 'src/lua/script_api.lua', output: 'script_api.lua', copy:true)
