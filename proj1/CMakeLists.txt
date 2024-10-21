cmake_minimum_required(VERSION 3.22)
project(Processor)

include_directories(/usr/include)

find_package(Python3 COMPONENTS Development REQUIRED)
include_directories(${Python3_INCLUDE_DIRS})
include_directories(matplotlib-cpp)

# Set SFML_DIR to the directory containing SFMLConfig.cmake
set(SFML_DIR "/SFML-2.6.1/lib/cmake/SFML")
find_package(SFML COMPONENTS graphics window system audio REQUIRED)
include_directories(${SFML_INCLUDE_DIR})

set(OpenCV_DIR "/opencv-4.x/cmake")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS} include)

add_executable(textProcessor textprocessor.cpp)
add_executable(audioProcessor audioprocessor.cpp)
add_executable(imageProcessor imageprocessor.cpp)

target_link_libraries(audioProcessor fftw3)

target_link_libraries(textProcessor ${Python3_LIBRARIES})
target_link_libraries(audioProcessor sfml-graphics sfml-window sfml-system sfml-audio ${FFTW_LIBRARIES})
target_link_libraries(imageProcessor ${OpenCV_LIBS} ${Python3_LIBRARIES})