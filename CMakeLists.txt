#TODO : support static linking

cmake_minimum_required(VERSION 3.10)

project(KewlEditor)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(SRCS 
	"${SRC_DIR}/main.c" 
	"${SRC_DIR}/TextBox.c"
	"${SRC_DIR}/TextBox.h" 
	"${SRC_DIR}/TextLine.c" 
	"${SRC_DIR}/TextLine.h"
	"${SRC_DIR}/OS.c" 
	"${SRC_DIR}/OS.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/UTF8String/UTFString.c" 
	"${CMAKE_CURRENT_SOURCE_DIR}/UTF8String/UTFString.h"
	)

#add os specific sources
if(UNIX)
	list(APPEND SRCS "${SRC_DIR}/linux/LinuxMain.c" "${SRC_DIR}/linux/LinuxMain.h")
endif()
if(WIN32)
	#TODO append windows main and library
endif()

add_executable(KewlEditor ${SRCS}) 

target_include_directories(KewlEditor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/UTF8String/")

#link and include os specific libraries
if(UNIX)
	target_link_libraries(KewlEditor X11)
	target_include_directories(KewlEditor PRIVATE X11)
endif()
if(WIN32)
	#TODO append windows main and library
endif()


#To be honest I am not sure which standard to use...
set_property(TARGET KewlEditor PROPERTY C_STANDARD 11)

#find sdl2 libraries

#load cmake find modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)

#try to find the sdl package
find_package(SDL2)
#try to find the sdl2_ttf package 
find_package(SDL2_ttf 2.20.0)

	
if((NOT SDL2_FOUND) OR (NOT SDL2_ttf_FOUND))
	if(NOT SDL2_FOUND)
		message(SEND_ERROR "Cmake couldn't find SDL2")
	endif()
	if(NOT SDL2_ttf_FOUND)
		message(SEND_ERROR "Cmake couldn't find SDL2_ttf")
	endif()
	message("")
	message("Try setting paths to SDL libraries manually by changing CMakeCache values")
	message("You can do so by using ccmake in terminal or by using cmake-gui")
	message("")

	########################
	#if find_package fails
	#we set
	#
	#  SDL2_INCLUDE_DIR
	#  SDL2_LIBRARY
	#  SDL2MAIN_LIBRAY
	#  
	#  SDL2_TTF_INCLUDE_DIR
	#  SDL2_TTF_LIBRARY
	#
	#to non advanced so it's easily visible to cmake gui
	########################

	mark_as_advanced(CLEAR 
		SDL2_INCLUDE_DIR
		SDL2_LIBRARY

		SDL2_TTF_INCLUDE_DIR
		SDL2_TTF_LIBRARY
	)
endif()

#link to sdl
target_include_directories(KewlEditor PRIVATE ${SDL2_INCLUDE_DIR})
#for ttf thing
target_include_directories(KewlEditor PRIVATE "${SDL2_INCLUDE_DIR}/SDL2")
target_link_libraries(KewlEditor "${SDL2_LIBRARY}")

#and ttf
target_include_directories(KewlEditor PRIVATE ${SDL2_TTF_INCLUDE_DIR})	
target_link_libraries(KewlEditor ${SDL2_TTF_LIBRARY})

#copy font to directory
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/NotoSansKR-Medium.otf" "${CMAKE_CURRENT_BINARY_DIR}/NotoSansKR-Medium.otf" COPYONLY)