project('NonEuclidean', 'cpp', default_options: ['warning_level=3', 'cpp_std=c++14'])

gl_dep = dependency('gl')

glew_dep = dependency('glew', method: 'pkg-config', required: false)
if not glew_dep.found()
  glew_dep = dependency('glew', static: true, required: true)
endif

game_objects_src = [
  'NonEuclidean/Player.cpp',
  'NonEuclidean/Portal.cpp',
]
scenes_src = [
  'NonEuclidean/Level1.cpp',
  'NonEuclidean/Level2.cpp',
  'NonEuclidean/Level3.cpp', 
  'NonEuclidean/Level4.cpp', 
  'NonEuclidean/Level5.cpp', 
  'NonEuclidean/Level6.cpp',
]
engine_src = [
  'NonEuclidean/Camera.cpp',
  'NonEuclidean/Collider.cpp',
  'NonEuclidean/Engine.cpp',
  'NonEuclidean/FrameBuffer.cpp',
  'NonEuclidean/Input.cpp',
  'NonEuclidean/Main.cpp',
  'NonEuclidean/Mesh.cpp',
  'NonEuclidean/Object.cpp',
  'NonEuclidean/Physical.cpp',
  'NonEuclidean/Resources.cpp',
  'NonEuclidean/Shader.cpp',
  'NonEuclidean/Texture.cpp',
]

executable('NonEuclidean',
  dependencies: [gl_dep, glew_dep],
  include_directories: include_directories('NonEuclidean'),
  gui_app: true,
  sources: game_objects_src + scenes_src + engine_src)
