# vendor/CMakeLists.txt

include(FetchContent)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
        GIT_TAG          origin/docking
)

add_subdirectory(imgui)