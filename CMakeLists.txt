cmake_minimum_required(VERSION 3.20)

# 集成vcpkg
# --- 核心集成代码开始 ---
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(myProject)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# 启用Qt的自动MOC、UIC、RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找依赖库
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(portaudio CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(podofo CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(CLI11 REQUIRED)

# Qt依赖（必需，vcpkg的qtbase是Qt6）
find_package(Qt6 COMPONENTS Core Widgets Gui QUIET)
# 查找 GoogleTest
find_package(GTest CONFIG REQUIRED)

# 将源文件打包进一个变量
set(COMMON_SOURCES
    src/common/logger.cpp
    src/common/config.cpp
    src/common/protocol.cpp
    src/common/interview_state.cpp
)
set(SERVICES_SOURCES
    src/services/realtime_client.cpp
    src/services/audio_manager.cpp
    src/services/llm_client.cpp
    src/services/pdf_parser.cpp
)
set(INTERVIEW_SOURCES
    src/interview/interview_manager.cpp
    src/interview/dialog_session.cpp
)
set(UI_SOURCES
    src/ui/mainwindow.cpp
    src/ui/config_dialog.cpp
)
set(UI_HEADERS
    include/ui/mainwindow.h
    include/ui/config_dialog.h
)

set(MAIN_SOURCES
		src/main.cpp
	)

set(SOURCES
    ${COMMON_SOURCES}
    ${SERVICES_SOURCES}
    ${INTERVIEW_SOURCES}
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${MAIN_SOURCES}
)

set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/common
    ${CMAKE_CURRENT_SOURCE_DIR}/include/services
    ${CMAKE_CURRENT_SOURCE_DIR}/include/interview
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ui
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${INCLUDE_DIRS}
)

# 关键：链接库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    # OpenSSL
    OpenSSL::SSL
    OpenSSL::Crypto
    # Boost
    Boost::system
    Boost::thread
    # JSON
    nlohmann_json::nlohmann_json
    # 压缩
    ZLIB::ZLIB
    # 音频
    portaudio
    # HTTP客户端
    CURL::libcurl
    # PDF解析
    podofo::podofo
    # 日志
    spdlog::spdlog
    # Qt界面
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    # 单元测试 (新加)
    GTest::gtest
    GTest::gtest_main

    CLI11::CLI11
)