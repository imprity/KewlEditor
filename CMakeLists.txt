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
	"${SRC_DIR}/linux/LinuxMain.c"
	"${SRC_DIR}/linux/LinuxMain.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/UTF8String/UTFString.c" 
	"${CMAKE_CURRENT_SOURCE_DIR}/UTF8String/UTFString.h"
	)

add_executable(KewlEditor ${SRCS}) 

target_include_directories(KewlEditor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/UTF8String/")

target_link_libraries(KewlEditor X11)

target_include_directories(KewlEditor PRIVATE X11)

#To be honest I am not sure which standard to use...
set_property(TARGET KewlEditor PROPERTY C_STANDARD 11)

#try to find the sdl package
find_package(SDL2)

#try to find the sdl2_ttf package 
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)

find_package(SDL2_ttf 2.20.0)

if(SDL2_FOUND AND SDL2_ttf_FOUND)
	#link to sdl
	target_include_directories(KewlEditor PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(KewlEditor "${SDL2_LIBRARIES}")

	#and ttf
	target_link_libraries(KewlEditor ${SDL2_TTF_LIBRARIES})
	target_include_directories(KewlEditor PRIVATE ${SDL2_TTF_INCLUDE_DIR})	
else()
	#You might be wondering why I link to both sdl and sdl ttf statically when either one fails.
	#Well I once tried to link to only failed one as an backup but there were a enough differences between sdl versions
	#that caused a fail at linking stage
	if(NOT SDL2_FOUND)
		message("Could not find SDL2 Package. Trying to link to built in static lib")
	endif()
	if(NOT SDL2_ttf_FOUND)
		message("Could not find SDL2_ttf Package. Trying to link to built in static lib")
	endif()

	#link to sdl
	add_library(SDL2 STATIC IMPORTED)

	set(SDL_DEPENDENCIES pthread m dl rt)

	set_target_properties(SDL2 PROPERTIES
		IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.24.0/lib/libSDL2.a"
		INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.24.0/include"
		INTERFACE_LINK_LIBRARIES "${SDL_DEPENDENCIES}"
		)

	add_library(SDL2_main STATIC IMPORTED)
	set_target_properties(SDL2_main PROPERTIES
		IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.24.0/lib/libSDL2main.a"
		)
	target_link_libraries(KewlEditor SDL2)
	target_link_libraries(KewlEditor SDL2_main)

	#and ttf
	add_library(freetype STATIC IMPORTED)
	set_target_properties(freetype PROPERTIES
		IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf-2.20.1/lib/libfreetype.a"
		)

	set(SDL2_ttf_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf-2.20.1/include" "${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.24.0/include/SDL2")
	add_library(SDL2_ttf STATIC IMPORTED)
	set_target_properties(SDL2_ttf PROPERTIES
		IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf-2.20.1/lib/libSDL2_ttf.a"
		INTERFACE_INCLUDE_DIRECTORIES "${SDL2_ttf_INCLUDE_DIRS}"
		INTERFACE_LINK_LIBRARIES freetype
		)

	target_link_libraries(KewlEditor SDL2_ttf)
endif()


#copy font to directory
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/NotoSansKR-Medium.otf" "${CMAKE_CURRENT_BINARY_DIR}/NotoSansKR-Medium.otf" COPYONLY)
