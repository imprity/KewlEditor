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

#load cmake find modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)

#try to find the sdl package
find_package(SDL2)
#try to find the sdl2_ttf package 
find_package(SDL2_ttf 2.20.0)

if(SDL2_FOUND AND SDL2_ttf_FOUND)
	#link to sdl
	target_include_directories(KewlEditor PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(KewlEditor "${SDL2_LIBRARIES}")

	#and ttf
	target_link_libraries(KewlEditor ${SDL2_TTF_LIBRARIES})
	target_include_directories(KewlEditor PRIVATE ${SDL2_TTF_INCLUDE_DIR})	
else()
	if(NOT SDL2_FOUND)
		message(SEND_ERROR "Cmake couldn't find SDL2")
		message(SEND_ERROR "Set the SDL2_PATH to path where sdl2 library is.")
	endif()
	if(NOT SDL2_ttf_FOUND)
		message(SEND_ERROR "Cmake couldn't find SDL2_ttf")
		message(SEND_ERROR "Set the SDL2_TTF_PATH to path where sdl2_ttf library is.")
	endif()
	return()
endif()

#copy font to directory
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/NotoSansKR-Medium.otf" "${CMAKE_CURRENT_BINARY_DIR}/NotoSansKR-Medium.otf" COPYONLY)