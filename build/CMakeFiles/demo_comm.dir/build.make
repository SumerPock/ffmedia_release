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
CMAKE_SOURCE_DIR = /home/firefly/ffmedia_release

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/firefly/ffmedia_release/build

# Include any dependencies generated for this target.
include CMakeFiles/demo_comm.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/demo_comm.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/demo_comm.dir/flags.make

CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.o: CMakeFiles/demo_comm.dir/flags.make
CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.o: ../demo/demo_comm.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/firefly/ffmedia_release/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.o -c /home/firefly/ffmedia_release/demo/demo_comm.cpp

CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/firefly/ffmedia_release/demo/demo_comm.cpp > CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.i

CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/firefly/ffmedia_release/demo/demo_comm.cpp -o CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.s

CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.o: CMakeFiles/demo_comm.dir/flags.make
CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.o: ../demo/ttyHandler.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/firefly/ffmedia_release/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.o -c /home/firefly/ffmedia_release/demo/ttyHandler.cpp

CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/firefly/ffmedia_release/demo/ttyHandler.cpp > CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.i

CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/firefly/ffmedia_release/demo/ttyHandler.cpp -o CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.s

CMakeFiles/demo_comm.dir/demo/netHandler.cpp.o: CMakeFiles/demo_comm.dir/flags.make
CMakeFiles/demo_comm.dir/demo/netHandler.cpp.o: ../demo/netHandler.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/firefly/ffmedia_release/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/demo_comm.dir/demo/netHandler.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/demo_comm.dir/demo/netHandler.cpp.o -c /home/firefly/ffmedia_release/demo/netHandler.cpp

CMakeFiles/demo_comm.dir/demo/netHandler.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/demo_comm.dir/demo/netHandler.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/firefly/ffmedia_release/demo/netHandler.cpp > CMakeFiles/demo_comm.dir/demo/netHandler.cpp.i

CMakeFiles/demo_comm.dir/demo/netHandler.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/demo_comm.dir/demo/netHandler.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/firefly/ffmedia_release/demo/netHandler.cpp -o CMakeFiles/demo_comm.dir/demo/netHandler.cpp.s

CMakeFiles/demo_comm.dir/demo/ini.c.o: CMakeFiles/demo_comm.dir/flags.make
CMakeFiles/demo_comm.dir/demo/ini.c.o: ../demo/ini.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/firefly/ffmedia_release/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/demo_comm.dir/demo/ini.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/demo_comm.dir/demo/ini.c.o   -c /home/firefly/ffmedia_release/demo/ini.c

CMakeFiles/demo_comm.dir/demo/ini.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/demo_comm.dir/demo/ini.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/firefly/ffmedia_release/demo/ini.c > CMakeFiles/demo_comm.dir/demo/ini.c.i

CMakeFiles/demo_comm.dir/demo/ini.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/demo_comm.dir/demo/ini.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/firefly/ffmedia_release/demo/ini.c -o CMakeFiles/demo_comm.dir/demo/ini.c.s

CMakeFiles/demo_comm.dir/demo/system_common.cpp.o: CMakeFiles/demo_comm.dir/flags.make
CMakeFiles/demo_comm.dir/demo/system_common.cpp.o: ../demo/system_common.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/firefly/ffmedia_release/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/demo_comm.dir/demo/system_common.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/demo_comm.dir/demo/system_common.cpp.o -c /home/firefly/ffmedia_release/demo/system_common.cpp

CMakeFiles/demo_comm.dir/demo/system_common.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/demo_comm.dir/demo/system_common.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/firefly/ffmedia_release/demo/system_common.cpp > CMakeFiles/demo_comm.dir/demo/system_common.cpp.i

CMakeFiles/demo_comm.dir/demo/system_common.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/demo_comm.dir/demo/system_common.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/firefly/ffmedia_release/demo/system_common.cpp -o CMakeFiles/demo_comm.dir/demo/system_common.cpp.s

# Object files for target demo_comm
demo_comm_OBJECTS = \
"CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.o" \
"CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.o" \
"CMakeFiles/demo_comm.dir/demo/netHandler.cpp.o" \
"CMakeFiles/demo_comm.dir/demo/ini.c.o" \
"CMakeFiles/demo_comm.dir/demo/system_common.cpp.o"

# External object files for target demo_comm
demo_comm_EXTERNAL_OBJECTS =

demo_comm: CMakeFiles/demo_comm.dir/demo/demo_comm.cpp.o
demo_comm: CMakeFiles/demo_comm.dir/demo/ttyHandler.cpp.o
demo_comm: CMakeFiles/demo_comm.dir/demo/netHandler.cpp.o
demo_comm: CMakeFiles/demo_comm.dir/demo/ini.c.o
demo_comm: CMakeFiles/demo_comm.dir/demo/system_common.cpp.o
demo_comm: CMakeFiles/demo_comm.dir/build.make
demo_comm: CMakeFiles/demo_comm.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/firefly/ffmedia_release/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Linking CXX executable demo_comm"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/demo_comm.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/demo_comm.dir/build: demo_comm

.PHONY : CMakeFiles/demo_comm.dir/build

CMakeFiles/demo_comm.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/demo_comm.dir/cmake_clean.cmake
.PHONY : CMakeFiles/demo_comm.dir/clean

CMakeFiles/demo_comm.dir/depend:
	cd /home/firefly/ffmedia_release/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/firefly/ffmedia_release /home/firefly/ffmedia_release /home/firefly/ffmedia_release/build /home/firefly/ffmedia_release/build /home/firefly/ffmedia_release/build/CMakeFiles/demo_comm.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/demo_comm.dir/depend

