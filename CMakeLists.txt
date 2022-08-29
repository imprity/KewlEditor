cmake_minimum_required(VERSION 3.21)

project(KewlEditor)

set(SRCS main.c TextBox.c TextBox.h TextLine.c TextLine.h 
"${CMAKE_CURRENT_SOURCE_DIR}/UTF8String/UTFString.c" 
"${CMAKE_CURRENT_SOURCE_DIR}/UTF8String/UTFString.h")

add_executable(KewlEditor ${SRCS}) 

target_include_directories(KewlEditor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/UTF8String/")

#To be honest I am not sure which standard to use...
set_property(TARGET KewlEditor PROPERTY C_STANDARD 17)

#try to find the sdl package
find_package(SDL2)

if(SDL2_FOUND)
	target_include_directories(KewlEditor PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(KewlEditor "${SDL2_LIBRARIES}")
else()
	#if we could not find sdl package then try to link with built in sdl statically
	message("Could not find SDL2 Package. Trying to link to built in static lib")
	find_package(SDL2 REQUIRED
		NO_MODULE
		PATHS "${CMAKE_CURRENT_SOURCE_DIR}/SDL2-24.0/lib/cmkae/SDL2"
		)
	target_include_directories(KewlEditor PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(KewlEditor "${SDL2_STATIC_LIBRARIES}")
endif()

#try to find the sdl2_ttf package 
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)

find_package(SDL2_ttf 2.20.0)

#if we could not find sdl2_ttf package, then try to link with built in sdl2_ttf statically

if(NOT SDL2_ttf_FOUND)
	add_library(SDL2_ttf STATIC IMPORTED)
	message("Could not find SDL2_ttf Package. Trying to link to built in static lib")
	set_target_properties(SDL2_ttf PROPERTIES
		IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf-2.20.1/lib/libSDL2_ttf.a"
		INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf-2.20.1/include"
		)

	add_library(freetype STATIC IMPORTED)
	set_target_properties(freetype PROPERTIES
		IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf-2.20.1/lib/libfreetype.a"
		)
	target_link_libraries(SDL2_ttf INTERFACE freetype)
	target_link_libraries(KewlEditor SDL2_ttf)
endif()

#copy font to directory
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/NotoSansKR-Medium.otf" "${CMAKE_CURRENT_BINARY_DIR}/NotoSansKR-Medium.otf" COPYONLY)