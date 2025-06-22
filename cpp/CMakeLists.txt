cmake_minimum_required(VERSION 3.18)
project(PocketGear LANGUAGES CXX)

# 指定 NDK 工具链（如果通过 android.toolchain.cmake 加载则不需要）
if(ANDROID)
    set(CMAKE_STRIP llvm-strip)  # 使用 NDK 提供的 llvm-strip
endif()

# 严格模式
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 全局编译选项
add_compile_options(
        -fno-stack-protector
        -fPIC
        -fno-rtti
        -fno-exceptions
        -Wno-attributes
        -Os
        -ffunction-sections
        -fdata-sections
)

# Android 特定设置
if(ANDROID)
    add_compile_options(-fno-addrsig)  # 减少 Android 上的符号大小
endif()

# 构建共享库
add_library(PocketGear SHARED PocketGear.cpp)

# 链接选项
target_link_options(PocketGear PRIVATE
        -Wl,--gc-sections
        -Wl,--as-needed
        -Wl,--strip-all
)

# 后处理：使用正确的 strip 工具
if(UNIX AND NOT APPLE)
    add_custom_command(TARGET PocketGear POST_BUILD
            COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:PocketGear>
            COMMENT "Stripping all symbols"
    )
endif()