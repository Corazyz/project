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

# Utility rule file for opencv_samples_tapi.

# Include the progress variables for this target.
include samples/tapi/CMakeFiles/opencv_samples_tapi.dir/progress.make

opencv_samples_tapi: samples/tapi/CMakeFiles/opencv_samples_tapi.dir/build.make

.PHONY : opencv_samples_tapi

# Rule to build all files generated by this target.
samples/tapi/CMakeFiles/opencv_samples_tapi.dir/build: opencv_samples_tapi

.PHONY : samples/tapi/CMakeFiles/opencv_samples_tapi.dir/build

samples/tapi/CMakeFiles/opencv_samples_tapi.dir/clean:
	cd /home/zyz/projects/opencv/opencv-4.10.0/build/samples/tapi && $(CMAKE_COMMAND) -P CMakeFiles/opencv_samples_tapi.dir/cmake_clean.cmake
.PHONY : samples/tapi/CMakeFiles/opencv_samples_tapi.dir/clean

samples/tapi/CMakeFiles/opencv_samples_tapi.dir/depend:
	cd /home/zyz/projects/opencv/opencv-4.10.0/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zyz/projects/opencv/opencv-4.10.0 /home/zyz/projects/opencv/opencv-4.10.0/samples/tapi /home/zyz/projects/opencv/opencv-4.10.0/build /home/zyz/projects/opencv/opencv-4.10.0/build/samples/tapi /home/zyz/projects/opencv/opencv-4.10.0/build/samples/tapi/CMakeFiles/opencv_samples_tapi.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : samples/tapi/CMakeFiles/opencv_samples_tapi.dir/depend
