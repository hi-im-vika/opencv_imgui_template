cmake_minimum_required(VERSION 3.10)
project(opencv_imgui_template)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCE "src/*.cpp")
file(GLOB HEADER "include/*.hpp")

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(OpenGL_GL_PREFERENCE GLVND)

add_subdirectory(vendor)
set(OpenCV_STATIC ON)
find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(spdlog REQUIRED)

add_definitions(-DWINDOW_NAME="${CMAKE_PROJECT_NAME}")

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE})
target_link_libraries(${CMAKE_PROJECT_NAME} ${OpenCV_LIBS} ${OPENGL_LIBRARY} imgui spdlog::spdlog SDL2::SDL2)