# Install script for directory: /home/zyz/projects/opencv/opencv-4.10.0/samples/cpp

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/opencv4/samples/cpp" TYPE FILE FILES
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/3calibration.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/CMakeLists.txt"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/application_trace.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/aruco_dict_utils.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/asift.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/audio_spectrogram.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/barcode.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/bgfg_segm.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/calibration.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/camshiftdemo.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/cloning_demo.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/cloning_gui.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/connected_components.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/contours2.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/convexhull.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/cout_mat.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/create_mask.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/dbt_face_detection.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/delaunay2.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/demhist.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/detect_blob.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/detect_mser.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/dft.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/digits_lenet.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/digits_svm.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/dis_opticalflow.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/distrans.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/drawing.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/edge.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/ela.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/em.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/epipolar_lines.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/essential_mat_reconstr.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/ex2_1.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/facedetect.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/facial_features.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/falsecolor.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/fback.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/ffilldemo.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/filestorage.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/fitellipse.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/flann_search_dataset.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/grabcut.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/image_alignment.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/imagelist_creator.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/imagelist_reader.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/imgcodecs_jpeg.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/inpaint.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/intelligent_scissors.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/intersectExample.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/kalman.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/kmeans.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/laplace.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/letter_recog.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/lkdemo.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/logistic_regression.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/lsd_lines.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/mask_tmpl.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/matchmethod_orb_akaze_brisk.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/minarea.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/morphology2.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/neural_network.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/npr_demo.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/opencv_version.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/pca.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/peopledetect.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/phase_corr.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/points_classifier.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/polar_transforms.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/qrcode.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/segment_objects.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/select3dobj.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/simd_basic.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/smiledetect.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/squares.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/stereo_calib.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/stereo_match.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/stitching.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/stitching_detailed.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/text_skewness_correction.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/train_HOG.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/train_svmsgd.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/travelsalesman.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/tree_engine.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_audio.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_audio_combination.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_basic.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_camera.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_gphoto2_autofocus.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_gstreamer_pipeline.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_image_sequence.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_microphone.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_obsensor.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_openni.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_realsense.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videocapture_starter.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/videowriter_basic.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/warpPerspective_demo.cpp"
    "/home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/watershed.cpp"
    )
endif()

