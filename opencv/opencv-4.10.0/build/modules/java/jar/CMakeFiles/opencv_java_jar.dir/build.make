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

# Utility rule file for opencv_java_jar.

# Include the progress variables for this target.
include modules/java/jar/CMakeFiles/opencv_java_jar.dir/progress.make

modules/java/jar/CMakeFiles/opencv_java_jar: bin/opencv-4100.jar


bin/opencv-4100.jar: modules/java/jar/CMakeFiles/opencv_java_jar.dir/java_class_filelist
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/zyz/projects/opencv/opencv-4.10.0/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Creating Java archive opencv-4100.jar"
	cd /home/zyz/projects/opencv/opencv-4.10.0/build/modules/java/jar/CMakeFiles/opencv_java_jar.dir && /usr/bin/jar -cfm /home/zyz/projects/opencv/opencv-4.10.0/build/bin/opencv-4100.jar /home/zyz/projects/opencv/opencv-4.10.0/build/modules/java/jar/opencv/MANIFEST.MF @java_class_filelist
	cd /home/zyz/projects/opencv/opencv-4.10.0/build/modules/java/jar/CMakeFiles/opencv_java_jar.dir && /usr/bin/cmake -D_JAVA_TARGET_DIR=/home/zyz/projects/opencv/opencv-4.10.0/build/bin -D_JAVA_TARGET_OUTPUT_NAME=opencv-4100.jar -D_JAVA_TARGET_OUTPUT_LINK= -P /usr/share/cmake-3.16/Modules/UseJavaSymlinks.cmake

modules/java/jar/CMakeFiles/opencv_java_jar.dir/java_class_filelist: modules/java/jar/CMakeFiles/opencv_java_jar.dir/java_compiled_opencv_java_jar
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/zyz/projects/opencv/opencv-4.10.0/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Generating CMakeFiles/opencv_java_jar.dir/java_class_filelist"
	cd /home/zyz/projects/opencv/opencv-4.10.0/modules/java/jar && /usr/bin/cmake -DCMAKE_JAVA_CLASS_OUTPUT_PATH=/home/zyz/projects/opencv/opencv-4.10.0/build/modules/java/jar/CMakeFiles/opencv_java_jar.dir -DCMAKE_JAR_CLASSES_PREFIX="" -P /usr/share/cmake-3.16/Modules/UseJavaClassFilelist.cmake

modules/java/jar/CMakeFiles/opencv_java_jar.dir/java_compiled_opencv_java_jar: modules/java/jar/opencv/java_sources
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/zyz/projects/opencv/opencv-4.10.0/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building Java objects for opencv_java_jar.jar"
	cd /home/zyz/projects/opencv/opencv-4.10.0/modules/java/jar && /usr/bin/javac -encoding utf-8 -classpath :/home/zyz/projects/opencv/opencv-4.10.0/modules/java/jar:/home/zyz/projects/opencv/opencv-4.10.0/build/lib -d /home/zyz/projects/opencv/opencv-4.10.0/build/modules/java/jar/CMakeFiles/opencv_java_jar.dir @/home/zyz/projects/opencv/opencv-4.10.0/build/modules/java/jar/opencv/java_sources
	cd /home/zyz/projects/opencv/opencv-4.10.0/modules/java/jar && /usr/bin/cmake -E touch /home/zyz/projects/opencv/opencv-4.10.0/build/modules/java/jar/CMakeFiles/opencv_java_jar.dir/java_compiled_opencv_java_jar

opencv_java_jar: modules/java/jar/CMakeFiles/opencv_java_jar
opencv_java_jar: bin/opencv-4100.jar
opencv_java_jar: modules/java/jar/CMakeFiles/opencv_java_jar.dir/java_class_filelist
opencv_java_jar: modules/java/jar/CMakeFiles/opencv_java_jar.dir/java_compiled_opencv_java_jar
opencv_java_jar: modules/java/jar/CMakeFiles/opencv_java_jar.dir/build.make

.PHONY : opencv_java_jar

# Rule to build all files generated by this target.
modules/java/jar/CMakeFiles/opencv_java_jar.dir/build: opencv_java_jar

.PHONY : modules/java/jar/CMakeFiles/opencv_java_jar.dir/build

modules/java/jar/CMakeFiles/opencv_java_jar.dir/clean:
	cd /home/zyz/projects/opencv/opencv-4.10.0/build/modules/java/jar && $(CMAKE_COMMAND) -P CMakeFiles/opencv_java_jar.dir/cmake_clean.cmake
.PHONY : modules/java/jar/CMakeFiles/opencv_java_jar.dir/clean

modules/java/jar/CMakeFiles/opencv_java_jar.dir/depend:
	cd /home/zyz/projects/opencv/opencv-4.10.0/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zyz/projects/opencv/opencv-4.10.0 /home/zyz/projects/opencv/opencv-4.10.0/modules/java/jar /home/zyz/projects/opencv/opencv-4.10.0/build /home/zyz/projects/opencv/opencv-4.10.0/build/modules/java/jar /home/zyz/projects/opencv/opencv-4.10.0/build/modules/java/jar/CMakeFiles/opencv_java_jar.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : modules/java/jar/CMakeFiles/opencv_java_jar.dir/depend

