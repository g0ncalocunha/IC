
set(CMAKE_BUILD_TYPE "Release")

set(BUILD_SHARED_LIBS "ON")

set(CMAKE_C_FLAGS "   -fsigned-char -W -Wall -Wreturn-type -Waddress -Wsequence-point -Wformat -Wformat-security -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wundef -Winit-self -Wpointer-arith -Wshadow -Wuninitialized -Wno-comment -Wimplicit-fallthrough=3 -Wno-strict-overflow -fdiagnostics-show-option -Wno-long-long -pthread -fomit-frame-pointer -ffunction-sections -fdata-sections  -msse3 -fvisibility=hidden")

set(CMAKE_C_FLAGS_DEBUG "-g  -O0 -DDEBUG -D_DEBUG")

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG  -DNDEBUG")

set(CMAKE_CXX_FLAGS "   -fsigned-char -W -Wall -Wreturn-type -Wnon-virtual-dtor -Waddress -Wsequence-point -Wformat -Wformat-security -Wmissing-declarations -Wundef -Winit-self -Wpointer-arith -Wshadow -Wsign-promo -Wuninitialized -Wsuggest-override -Wno-delete-non-virtual-dtor -Wno-comment -Wimplicit-fallthrough=3 -Wno-strict-overflow -fdiagnostics-show-option -Wno-long-long -pthread -fomit-frame-pointer -ffunction-sections -fdata-sections  -msse3 -fvisibility=hidden -fvisibility-inlines-hidden")

set(CMAKE_CXX_FLAGS_DEBUG "-g  -O0 -DDEBUG -D_DEBUG")

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG  -DNDEBUG")

set(CV_GCC "1")

set(CV_CLANG "")

set(ENABLE_NOISY_WARNINGS "OFF")

set(CMAKE_MODULE_LINKER_FLAGS "  -Wl,--gc-sections -Wl,--as-needed -Wl,--no-undefined")

set(CMAKE_INSTALL_PREFIX "/usr/local")

set(OPENCV_PYTHON_INSTALL_PATH "")

set(OpenCV_SOURCE_DIR "/home/goncalo/MECT1-1/IC/proj1/opencv-4.x")

set(OPENCV_FORCE_PYTHON_LIBS "")

set(OPENCV_PYTHON_SKIP_LINKER_EXCLUDE_LIBS "")

set(OPENCV_PYTHON_BINDINGS_DIR "/home/goncalo/MECT1-1/IC/proj1/opencv_build/modules/python_bindings_generator")

set(cv2_custom_hdr "/home/goncalo/MECT1-1/IC/proj1/opencv_build/modules/python_bindings_generator/pyopencv_custom_headers.h")

set(cv2_generated_files "/home/goncalo/MECT1-1/IC/proj1/opencv_build/modules/python_bindings_generator/pyopencv_generated_enums.h;/home/goncalo/MECT1-1/IC/proj1/opencv_build/modules/python_bindings_generator/pyopencv_generated_funcs.h;/home/goncalo/MECT1-1/IC/proj1/opencv_build/modules/python_bindings_generator/pyopencv_generated_include.h;/home/goncalo/MECT1-1/IC/proj1/opencv_build/modules/python_bindings_generator/pyopencv_generated_modules.h;/home/goncalo/MECT1-1/IC/proj1/opencv_build/modules/python_bindings_generator/pyopencv_generated_modules_content.h;/home/goncalo/MECT1-1/IC/proj1/opencv_build/modules/python_bindings_generator/pyopencv_generated_types.h;/home/goncalo/MECT1-1/IC/proj1/opencv_build/modules/python_bindings_generator/pyopencv_generated_types_content.h;/home/goncalo/MECT1-1/IC/proj1/opencv_build/modules/python_bindings_generator/pyopencv_signatures.json")