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
CMAKE_SOURCE_DIR = /home/zyz/projects/test_project/opencv/opencv_learn

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zyz/projects/test_project/opencv/opencv_learn/build

# Include any dependencies generated for this target.
include CMakeFiles/create_mat.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/create_mat.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/create_mat.dir/flags.make

CMakeFiles/create_mat.dir/create_mat.cpp.o: CMakeFiles/create_mat.dir/flags.make
CMakeFiles/create_mat.dir/create_mat.cpp.o: ../create_mat.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zyz/projects/test_project/opencv/opencv_learn/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/create_mat.dir/create_mat.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/create_mat.dir/create_mat.cpp.o -c /home/zyz/projects/test_project/opencv/opencv_learn/create_mat.cpp

CMakeFiles/create_mat.dir/create_mat.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/create_mat.dir/create_mat.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zyz/projects/test_project/opencv/opencv_learn/create_mat.cpp > CMakeFiles/create_mat.dir/create_mat.cpp.i

CMakeFiles/create_mat.dir/create_mat.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/create_mat.dir/create_mat.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zyz/projects/test_project/opencv/opencv_learn/create_mat.cpp -o CMakeFiles/create_mat.dir/create_mat.cpp.s

# Object files for target create_mat
create_mat_OBJECTS = \
"CMakeFiles/create_mat.dir/create_mat.cpp.o"

# External object files for target create_mat
create_mat_EXTERNAL_OBJECTS =

create_mat: CMakeFiles/create_mat.dir/create_mat.cpp.o
create_mat: CMakeFiles/create_mat.dir/build.make
create_mat: /usr/local/lib/libopencv_gapi.so.4.10.0
create_mat: /usr/local/lib/libopencv_highgui.so.4.10.0
create_mat: /usr/local/lib/libopencv_ml.so.4.10.0
create_mat: /usr/local/lib/libopencv_objdetect.so.4.10.0
create_mat: /usr/local/lib/libopencv_photo.so.4.10.0
create_mat: /usr/local/lib/libopencv_stitching.so.4.10.0
create_mat: /usr/local/lib/libopencv_video.so.4.10.0
create_mat: /usr/local/lib/libopencv_videoio.so.4.10.0
create_mat: /usr/local/lib/libopencv_imgcodecs.so.4.10.0
create_mat: /usr/local/lib/libopencv_dnn.so.4.10.0
create_mat: /usr/local/lib/libopencv_calib3d.so.4.10.0
create_mat: /usr/local/lib/libopencv_features2d.so.4.10.0
create_mat: /usr/local/lib/libopencv_flann.so.4.10.0
create_mat: /usr/local/lib/libopencv_imgproc.so.4.10.0
create_mat: /usr/local/lib/libopencv_core.so.4.10.0
create_mat: CMakeFiles/create_mat.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zyz/projects/test_project/opencv/opencv_learn/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable create_mat"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/create_mat.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/create_mat.dir/build: create_mat

.PHONY : CMakeFiles/create_mat.dir/build

CMakeFiles/create_mat.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/create_mat.dir/cmake_clean.cmake
.PHONY : CMakeFiles/create_mat.dir/clean

CMakeFiles/create_mat.dir/depend:
	cd /home/zyz/projects/test_project/opencv/opencv_learn/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zyz/projects/test_project/opencv/opencv_learn /home/zyz/projects/test_project/opencv/opencv_learn /home/zyz/projects/test_project/opencv/opencv_learn/build /home/zyz/projects/test_project/opencv/opencv_learn/build /home/zyz/projects/test_project/opencv/opencv_learn/build/CMakeFiles/create_mat.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/create_mat.dir/depend

