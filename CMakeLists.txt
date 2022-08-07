cmake_minimum_required(VERSION 3.0)

project(KewlEditor)

set(SRCS main.c stb_truetype.h UTFString.c UTFString.h TextBox.c TextBox.h)

add_executable(KewlEditor ${SRCS}) 

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/includes")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/includes/SDL2")

#TODO : Do I really need all this stuffs?
#TODO : Make option for x86 or 64
#TODO : Make option for other OSes (or you can just include source files and build with it)

#link to main sdl2 stuffs

set(SDL2_LIB "SDL2-2.0.18/lib/x64")

target_link_libraries(KewlEditor "${CMAKE_CURRENT_SOURCE_DIR}/${SDL2_LIB}/SDL2.lib")
target_link_libraries(KewlEditor "${CMAKE_CURRENT_SOURCE_DIR}/${SDL2_LIB}/SDL2main.lib")
target_link_libraries(KewlEditor "${CMAKE_CURRENT_SOURCE_DIR}/${SDL2_LIB}/SDL2test.lib")
target_link_libraries(KewlEditor "opengl32.lib")

#link to sdl2 image stuffs
set(SDL2_IMG_LIB "SDL2_image-devel-2.0.5-VC/SDL2_image-2.0.5/lib/x64")

target_link_libraries(KewlEditor "${CMAKE_CURRENT_SOURCE_DIR}/${SDL2_IMG_LIB}/SDL2_image.lib")

#link to sdl2 ttf stuffs
set(SDL2_TTF_LIB "SDL2_ttf-2.20.0/lib/x64")

target_link_libraries(KewlEditor "${CMAKE_CURRENT_SOURCE_DIR}/${SDL2_TTF_LIB}/SDL2_ttf.lib")

#TODO : make function to copy dlls to build folder (or won't because we will just build everything with our source code)
