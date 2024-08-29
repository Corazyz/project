# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/zyz/projects/opencv/opencv-4.10.0

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zyz/projects/opencv/opencv-4.10.0/build

# Include any dependencies generated for this target.
include samples/cpp/CMakeFiles/example_cpp_inpaint.dir/depend.make

# Include the progress variables for this target.
include samples/cpp/CMakeFiles/example_cpp_inpaint.dir/progress.make

# Include the compile flags for this target's objects.
include samples/cpp/CMakeFiles/example_cpp_inpaint.dir/flags.make

samples/cpp/CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.o: samples/cpp/CMakeFiles/example_cpp_inpaint.dir/flags.make
samples/cpp/CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.o: ../samples/cpp/inpaint.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zyz/projects/opencv/opencv-4.10.0/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object samples/cpp/CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.o"
	cd /home/zyz/projects/opencv/opencv-4.10.0/build/samples/cpp && /usr/bin/ccache /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.o -c /home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/inpaint.cpp

samples/cpp/CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.i"
	cd /home/zyz/projects/opencv/opencv-4.10.0/build/samples/cpp && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/inpaint.cpp > CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.i

samples/cpp/CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.s"
	cd /home/zyz/projects/opencv/opencv-4.10.0/build/samples/cpp && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zyz/projects/opencv/opencv-4.10.0/samples/cpp/inpaint.cpp -o CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.s

# Object files for target example_cpp_inpaint
example_cpp_inpaint_OBJECTS = \
"CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.o"

# External object files for target example_cpp_inpaint
example_cpp_inpaint_EXTERNAL_OBJECTS =

bin/example_cpp_inpaint: samples/cpp/CMakeFiles/example_cpp_inpaint.dir/inpaint.cpp.o
bin/example_cpp_inpaint: samples/cpp/CMakeFiles/example_cpp_inpaint.dir/build.make
bin/example_cpp_inpaint: 3rdparty/lib/libippiw.a
bin/example_cpp_inpaint: 3rdparty/ippicv/ippicv_lnx/icv/lib/intel64/libippicv.a
bin/example_cpp_inpaint: lib/libopencv_gapi.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_highgui.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_ml.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_objdetect.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_photo.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_stitching.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_video.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_videoio.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_imgcodecs.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_calib3d.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_features2d.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_flann.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_dnn.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_imgproc.so.4.10.0
bin/example_cpp_inpaint: lib/libopencv_core.so.4.10.0
bin/example_cpp_inpaint: samples/cpp/CMakeFiles/example_cpp_inpaint.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zyz/projects/opencv/opencv-4.10.0/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/example_cpp_inpaint"
	cd /home/zyz/projects/opencv/opencv-4.10.0/build/samples/cpp && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/example_cpp_inpaint.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
samples/cpp/CMakeFiles/example_cpp_inpaint.dir/build: bin/example_cpp_inpaint

.PHONY : samples/cpp/CMakeFiles/example_cpp_inpaint.dir/build

samples/cpp/CMakeFiles/example_cpp_inpaint.dir/clean:
	cd /home/zyz/projects/opencv/opencv-4.10.0/build/samples/cpp && $(CMAKE_COMMAND) -P CMakeFiles/example_cpp_inpaint.dir/cmake_clean.cmake
.PHONY : samples/cpp/CMakeFiles/example_cpp_inpaint.dir/clean

samples/cpp/CMakeFiles/example_cpp_inpaint.dir/depend:
	cd /home/zyz/projects/opencv/opencv-4.10.0/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zyz/projects/opencv/opencv-4.10.0 /home/zyz/projects/opencv/opencv-4.10.0/samples/cpp /home/zyz/projects/opencv/opencv-4.10.0/build /home/zyz/projects/opencv/opencv-4.10.0/build/samples/cpp /home/zyz/projects/opencv/opencv-4.10.0/build/samples/cpp/CMakeFiles/example_cpp_inpaint.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : samples/cpp/CMakeFiles/example_cpp_inpaint.dir/depend
