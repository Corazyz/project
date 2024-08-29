# Install script for directory: /home/zyz/projects/opencv/opencv-4.10.0/samples/dnn

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "RELEASE")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xsamplesx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/opencv4/samples/dnn" TYPE FILE FILES
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/CMakeLists.txt"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/classification.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/colorization.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/common.hpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/custom_layers.hpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/dasiamrpn_tracker.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/face_detect.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/human_parsing.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/nanotrack_tracker.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/object_detection.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/openpose.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/person_reid.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/scene_text_detection.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/scene_text_recognition.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/scene_text_spotting.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/segmentation.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/speech_recognition.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/text_detection.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/vit_tracker.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/dnn/yolo_detector.cpp"
    )
endif()

