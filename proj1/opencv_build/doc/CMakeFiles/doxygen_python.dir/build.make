# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/goncalo/MECT1-1/IC/proj1/opencv-4.x

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/goncalo/MECT1-1/IC/proj1/opencv_build

# Utility rule file for doxygen_python.

# Include any custom commands dependencies for this target.
include doc/CMakeFiles/doxygen_python.dir/compiler_depend.make

# Include the progress variables for this target.
include doc/CMakeFiles/doxygen_python.dir/progress.make

doc/CMakeFiles/doxygen_python:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/goncalo/MECT1-1/IC/proj1/opencv_build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Inject Python signatures into documentation"
	cd /home/goncalo/MECT1-1/IC/proj1/opencv_build/doc && /usr/bin/python3 /home/goncalo/MECT1-1/IC/proj1/opencv-4.x/doc/tools/add_signatures.py /home/goncalo/MECT1-1/IC/proj1/opencv_build/doc/doxygen/html/ /home/goncalo/MECT1-1/IC/proj1/opencv_build/modules/python_bindings_generator/pyopencv_signatures.json python

doxygen_python: doc/CMakeFiles/doxygen_python
doxygen_python: doc/CMakeFiles/doxygen_python.dir/build.make
.PHONY : doxygen_python

# Rule to build all files generated by this target.
doc/CMakeFiles/doxygen_python.dir/build: doxygen_python
.PHONY : doc/CMakeFiles/doxygen_python.dir/build

doc/CMakeFiles/doxygen_python.dir/clean:
	cd /home/goncalo/MECT1-1/IC/proj1/opencv_build/doc && $(CMAKE_COMMAND) -P CMakeFiles/doxygen_python.dir/cmake_clean.cmake
.PHONY : doc/CMakeFiles/doxygen_python.dir/clean

doc/CMakeFiles/doxygen_python.dir/depend:
	cd /home/goncalo/MECT1-1/IC/proj1/opencv_build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/goncalo/MECT1-1/IC/proj1/opencv-4.x /home/goncalo/MECT1-1/IC/proj1/opencv-4.x/doc /home/goncalo/MECT1-1/IC/proj1/opencv_build /home/goncalo/MECT1-1/IC/proj1/opencv_build/doc /home/goncalo/MECT1-1/IC/proj1/opencv_build/doc/CMakeFiles/doxygen_python.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : doc/CMakeFiles/doxygen_python.dir/depend

