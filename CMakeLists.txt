cmake_minimum_required(VERSION 3.20)

project("textshader" LANGUAGES C)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME} "textshader.c")
target_link_libraries(${PROJECT_NAME} SDL2::SDL2 OpenGL::OpenGL)
