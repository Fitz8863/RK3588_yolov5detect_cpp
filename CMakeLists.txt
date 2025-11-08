cmake_minimum_required(VERSION 3.4.1)

project(rknn_yolov5_demo)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-pthread")

# skip 3rd-party lib dependencies
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-shlib-undefined")

# install target and libraries
set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# rknn api 
set(RKNN_RT_LIB ${CMAKE_SOURCE_DIR}/lib/rknn/librknnrt.so)

#rga
set(RGA_LIB ${CMAKE_SOURCE_DIR}/lib/rga/librga.so)


# opencv
# find_package(OpenCV REQUIRED)
# set(CMAKE_INSTALL_RPATH "lib")

file(GLOB OpenCV_LIBS "${CMAKE_SOURCE_DIR}/lib/opencv/libopencv_*")
include_directories(${CMAKE_SOURCE_DIR}/include/opencv/opencv4)

message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

# rknn_yolov5_demo
include_directories(${CMAKE_SOURCE_DIR}/include/rknn)
include_directories(${CMAKE_SOURCE_DIR}/include/rknn/3rdparty/rga/include)

add_executable(rknn_yolov5_demo
        src/main.cc
        src/postprocess.cc
        src/preprocess.cc
        src/rkYolov5s.cc
)

target_link_libraries(rknn_yolov5_demo
  ${RKNN_RT_LIB}
  ${OpenCV_LIBS}
  ${RGA_LIB}
)


