cmake_minimum_required(VERSION 3.16)
project(MyQtFfplayer)

set(CMAKE_CXX_STANDARD 17)

#-----------------------------------------------------------
# 设置构建类型为 Debug，包含调试信息并关闭优化
#set(CMAKE_BUILD_TYPE Debug)
# 或者更保险地设置编译标志
#set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall")
#-----------------------------------------------------------


set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用 Qt 自动工具
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 定义 CONFIG_AVFILTER（启用 FFmpeg 滤镜）
add_definitions(-DCONFIG_AVFILTER)

# 可选：开启详细链接输出，方便调试
set(CMAKE_VERBOSE_MAKEFILE ON)

# ------------------------------------------------------------------
# 1. 查找 Qt6
# ------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)

# ------------------------------------------------------------------
# 2. 查找 FFmpeg（包含 libavfilter 和 libswresample）
# ------------------------------------------------------------------
find_package(PkgConfig REQUIRED)

set(FFMPEG_COMPONENTS
    libavcodec
    libavformat
    libavutil
    libswscale
    libavdevice
    libavfilter
    libswresample)

foreach(comp ${FFMPEG_COMPONENTS})
    pkg_check_modules(FFMPEG_${comp} REQUIRED ${comp})
endforeach()

# ------------------------------------------------------------------
# 3. 查找 SDL2
# ------------------------------------------------------------------
pkg_check_modules(SDL2 REQUIRED sdl2)


# ============ 本地 SoundTouch 构建 ============
set(LOCAL_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install")
set(SOUNDTOUCH_DLL ON CACHE BOOL "Build SoundTouch C interface" FORCE)
set(CMAKE_INSTALL_PREFIX "${LOCAL_INSTALL_PREFIX}" CACHE PATH "Install prefix" FORCE)

add_subdirectory(3rdparty/soundtouch)

if(TARGET SoundTouch)
    set(SOUNDTOUCH_TARGET SoundTouch)
elseif(TARGET soundtouch)
    set(SOUNDTOUCH_TARGET soundtouch)
else()
    message(FATAL_ERROR "Could not find SoundTouch library target.")
endif()

message(STATUS "Using SoundTouch target: ${SOUNDTOUCH_TARGET}")
# ==============================================




# ------------------------------------------------------------------
# 5. 添加 GLM 头文件路径
# ------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------
# 6. 收集源文件
# ------------------------------------------------------------------
# ------------------------------------------------------------------
# 6. 收集源文件
# ------------------------------------------------------------------
file(GLOB_RECURSE PLAYER_SOURCES
     "QtFfplayer/*.cpp"
     "QtFfplayer/*.c"
     "QtFfplayer/*.qrc"
)

# 手动添加 SoundTouchDLL.cpp（仅追加到源文件列表，不调用 target_* 命令）
set(SOUNDTOUCH_DLL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/soundtouch/source/SoundTouchDLL/SoundTouchDLL.cpp")
if(EXISTS ${SOUNDTOUCH_DLL_SRC})
    list(APPEND PLAYER_SOURCES ${SOUNDTOUCH_DLL_SRC})
    message(STATUS "Added SoundTouchDLL.cpp to player sources")
else()
    message(WARNING "SoundTouchDLL.cpp not found, C API will be missing")
endif()

file(GLOB_RECURSE PLAYER_HEADERS
     "QtFfplayer/*.h"
     "QtFfplayer/*.hpp"
)


# ------------------------------------------------------------------
# ============ 收集 RPATH 目录 ============
set(RPATH_DIRS "")

# 添加 Qt 库目录
get_target_property(QT_CORE_LOCATION Qt6::Core IMPORTED_LOCATION)
if(QT_CORE_LOCATION)
    get_filename_component(QT_LIB_DIR ${QT_CORE_LOCATION} DIRECTORY)
    list(APPEND RPATH_DIRS ${QT_LIB_DIR})
endif()

# 添加 FFmpeg 库目录
foreach(comp ${FFMPEG_COMPONENTS})
    set(libs_var FFMPEG_${comp}_LINK_LIBRARIES)
    foreach(lib ${${libs_var}})
        if(IS_ABSOLUTE ${lib})
            get_filename_component(lib_dir ${lib} DIRECTORY)
            list(APPEND RPATH_DIRS ${lib_dir})
        endif()
    endforeach()
endforeach()

if(RPATH_DIRS)
    list(REMOVE_DUPLICATES RPATH_DIRS)
    set(CMAKE_BUILD_RPATH "${RPATH_DIRS}")
    set(CMAKE_INSTALL_RPATH "${RPATH_DIRS}")
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    message(STATUS "RPATH directories: ${RPATH_DIRS}")
endif()

# =========================================

list(APPEND RPATH_DIRS ${LOCAL_INSTALL_PREFIX}/lib)

# 然后继续创建可执行文件
add_executable(QtFfplayer ${PLAYER_SOURCES} ${PLAYER_HEADERS})


# 现在目标已存在，可以设置其属性
# 为 QtFfplayer 定义 DLL_EXPORTS，使 SOUNDTOUCHDLL_API 展开为导出声明
target_compile_definitions(QtFfplayer PRIVATE DLL_EXPORTS)

# 添加 SoundTouch 头文件路径（确保编译时能找到 SoundTouch.h）
target_include_directories(QtFfplayer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/soundtouch/include)



# ------------------------------------------------------------------
# 7. 创建可执行文件
# ------------------------------------------------------------------
#add_executable(QtFfplayer ${PLAYER_SOURCES} ${PLAYER_HEADERS})

# 添加 SoundTouch 头文件路径（确保编译时能找到 SoundTouch.h）
#target_include_directories(QtFfplayer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/soundtouch/include)



# ================ GLM SIMD 配置 =================
option(ENABLE_GLM_SIMD "Enable GLM SIMD optimizations (requires SSE2)" OFF)
if(ENABLE_GLM_SIMD)
    target_compile_options(QtFfplayer PRIVATE -msse2)
    target_compile_definitions(QtFfplayer PRIVATE GLM_FORCE_SSE2)
    message(STATUS "GLM SIMD enabled (SSE2)")
else()
    target_compile_definitions(QtFfplayer PRIVATE GLM_FORCE_PURE)
    message(STATUS "GLM SIMD disabled (pure scalar)")
endif()

# 添加必要的预处理器定义
target_compile_definitions(QtFfplayer PRIVATE __STDC_CONSTANT_MACROS)

# ------------------------------------------------------------------
# 8. 链接库
# ------------------------------------------------------------------
# 链接 Qt 库（改为 PRIVATE）
target_link_libraries(QtFfplayer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

# 链接 FFmpeg 库（循环内也要加 PRIVATE）
foreach(comp ${FFMPEG_COMPONENTS})
    if(TARGET PkgConfig::FFMPEG_${comp})
        target_link_libraries(QtFfplayer PRIVATE PkgConfig::FFMPEG_${comp})
    else()
        target_link_libraries(QtFfplayer PRIVATE ${FFMPEG_${comp}_LINK_LIBRARIES})
        target_include_directories(QtFfplayer PRIVATE ${FFMPEG_${comp}_INCLUDE_DIRS})
    endif()
endforeach()

# 链接 SDL2（改为 PRIVATE）
target_link_libraries(QtFfplayer PRIVATE ${SDL2_LIBRARIES})
target_include_directories(QtFfplayer PRIVATE ${SDL2_INCLUDE_DIRS})

# 链接 SoundTouch（直接使用目标）
target_link_libraries(QtFfplayer PRIVATE ${SOUNDTOUCH_TARGET})

# ------------------------------------------------------------------
# 9. 输出提示
# ------------------------------------------------------------------
message(STATUS "Qt6 version: ${Qt6_VERSION}")
message(STATUS "FFmpeg libraries found.")
message(STATUS "SDL2 found: ${SDL2_FOUND}")
message(STATUS "SoundTouch found: ${SOUNDTOUCH_FOUND}")
message(STATUS "GLM include path: ${CMAKE_CURRENT_SOURCE_DIR}/glm")