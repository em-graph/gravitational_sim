cmake_minimum_required(VERSION 3.0...3.5)
project(PRJ_TEMPLATE VERSION 0.1.0)

set(CMAKE_C_STANDARD 99)#C99 # lvgl officially support C99 and above
set(CMAKE_CXX_STANDARD 20)#C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

# Add source files
aux_source_directory(./application/ SRC)
aux_source_directory(./library/bresenham SRC)
aux_source_directory(./library/bitmap SRC)
aux_source_directory(./library/lodepng SRC)
aux_source_directory(./library/linkedlist SRC)

add_executable(${PROJECT_NAME} ${SRC}
        application/color.h
)

# Local head file dirtion
include_directories(${HEAD_PATH})
include_directories("./library/bresenham")
include_directories("./library/bitmap")
include_directories("./library/linkedlist")
include_directories("./library/lodepng")
include_directories("./library")

# Linked opencv library
set(OpenCV_DIR "D:/opencv/mingw-build")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

# Linked boost library
set(BOOST_ROOT "D:/boost_1_89_0")  # Boost根目录路径
set(Boost_USE_STATIC_LIBS OFF)    # 使用动态库或静态库，根据需要设置
set(Boost_USE_MULTITHREADED ON)   # 使用多线程库
set(Boost_USE_STATIC_RUNTIME OFF) # 使用动态运行时库
#
find_package(Boost 1.89.0 REQUIRED) # 指定版本和组件
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

# Linked SDL2 library
find_package(SDL2 REQUIRED SDL2)
include_directories(${SDL2_INCLUDE_DIR})

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/library/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL2_INCLUDE_DIR}
)
target_link_libraries(${PROJECT_NAME} imgui ${SDL2_LIBRARIES})

