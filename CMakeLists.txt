cmake_minimum_required(VERSION 3.16)
project(QRCodeScanner VERSION 1.2.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set (ZXING_PUBLIC_FLAGS
    -DZXING_EXPERIMENTAL_API
)

if (MSVC)
    set (ZXING_PUBLIC_FLAGS ${ZXING_PUBLIC_FLAGS}
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
    )
endif()

set (ZXING_PRIVATE_FLAGS
    -DZXING_USE_ZINT
)
if (MSVC)
    set (ZXING_PRIVATE_FLAGS ${ZXING_PRIVATE_FLAGS}
        -D_SCL_SECURE_NO_WARNINGS
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_NONSTDC_NO_WARNINGS
        -DNOMINMAX
        /utf-8
    )
else()
    set (ZXING_PRIVATE_FLAGS ${ZXING_PRIVATE_FLAGS}
        -Wall -Wextra -Wno-missing-braces -Werror=undef -Werror=return-type)
endif()

include (CheckCXXCompilerFlag)

# This is needed for reproducible builds across different build directories.
# Without this, the usage of the __FILE__ macro leaves the build directory in
# the binary. When building the Python extension with build isolation enabled
# this would lead to random paths in the binary.
set(FILE_PREFIX_ARG "-fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=")
check_cxx_compiler_flag("${FILE_PREFIX_ARG}" HAS_FILE_PREFIX_ARG)
if(HAS_FILE_PREFIX_ARG)
    set(ZXING_PRIVATE_FLAGS ${ZXING_PRIVATE_FLAGS} "${FILE_PREFIX_ARG}")
endif()

# 向编译器添加编译选项
if (MSVC)
    set(VERSION_RC "${PROJECT_SOURCE_DIR}/version.rc")
    configure_file(
        "${PROJECT_SOURCE_DIR}/version.rc.in"
        "${VERSION_RC}"
    )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options("-W3")
endif()

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")
message("BUILD_TIMESTAMP: ${BUILD_TIMESTAMP}")

configure_file(
    "${PROJECT_SOURCE_DIR}/src/version.h.in"
    "${PROJECT_SOURCE_DIR}/src/version.h"
)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# 查找Qt组件与Linguist工具
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
if (Qt6_FOUND)
    message("Qt6 found!")
    set(QT_NAME Qt6)
elseif(Qt5_FOUND)
    message("Qt5 found!")
    set(QT_NAME Qt5)
else()
    message(FATAL_ERROR "Qt not found!")
    return()
endif()

find_package(${QT_NAME} REQUIRED COMPONENTS Core Gui Widgets
    Multimedia
    MultimediaWidgets
    LinguistTools
)

# 包含 ZXing 库
set(ZXING_DIR "${PROJECT_SOURCE_DIR}/ZXing-${CMAKE_BUILD_TYPE}")
include(${ZXING_DIR}/lib/cmake/ZXing/ZXingConfig.cmake)

qt_standard_project_setup(
    I18N_SOURCE_LANGUAGE zh_CN
    I18N_TRANSLATED_LANGUAGES en_US
)

# 构建Android需使用Qt Creator
# Android构建需添加 Android OpenSSL
if (ANDROID)
    if(DEFINED ENV{ANDROID_HOME})
        set(AndroidSDK $ENV{ANDROID_HOME})
    elseif(DEFINE ENV{ANDROID_SDK_ROOT})
        set(AndroidSDK $ENV{ANDROID_SDK_ROOT})
    else()
        # 没有找到SDK环境变量
        message(FATAL_ERROR "ANDROID SDK env not found!")
        return()
    endif()
    include(${AndroidSDK}/android_openssl/android_openssl.cmake)
endif()

# 源文件列表
set(SOURCES
    src/ImageView.cpp
    src/QRCodeScanner.cpp
    src/QRCodeGenerator.cpp
    src/main.cpp
)

# 头文件列表
set(HEADERS
    src/ImageView.h
    src/QRCodeScanner.h
    src/QRCodeGenerator.h
)

# UI 文件列表
set(UIS
    src/QRCodeScanner.ui
    src/QRCodeGenerator.ui
)

# Qt 资源文件
qt_add_resources(RES
    QRCodeScanner.qrc
)

# 生成可执行文件
qt_add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UIS} ${RES} ${VERSION_RC})

# 翻译文件
qt_add_translations(${PROJECT_NAME}
    SOURCES ${SOURCES} ${HEADERS} ${UIS}
    TS_FILE_BASE Translation
    TS_FILE_DIR langs
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_compile_options (${PROJECT_NAME}
    PUBLIC ${ZXING_PUBLIC_FLAGS}
    PRIVATE ${ZXING_PRIVATE_FLAGS}
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${QT_NAME}::Core
    ${QT_NAME}::Gui
    ${QT_NAME}::Widgets
    ${QT_NAME}::Multimedia
    ${QT_NAME}::MultimediaWidgets
    ZXing::Core
)

target_link_libraries (${PROJECT_NAME} PUBLIC Threads::Threads)

# 添加 Widgets 模块的头文件路径
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
    ${${QT_NAME}Widgets_INCLUDE_DIRS}
    ${${QT_NAME}Multimedia_INCLUDE_DIRS}
    ${${QT_NAME}MultimediaWidgets_INCLUDE_DIRS}
)

# 可选：添加 OpenCV 库
find_package(OpenCV QUIET)
if (OpenCV_FOUND)
    target_include_directories (${PROJECT_NAME} SYSTEM PUBLIC ${OpenCV_INCLUDE_DIRS})
    target_link_libraries (${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    include/
    src/
)

if (ANDROID)
    add_android_openssl_libraries(${PROJECT_NAME})
endif()

install(TARGETS ${PROJECT_NAME}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
