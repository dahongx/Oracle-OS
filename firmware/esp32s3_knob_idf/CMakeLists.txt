# 顶层 CMakeLists —— 跟 espp 官方 example 一样的套路:
# 关闭 IDF Component Manager(避免网络注册表拉取的各种坑),
# 直接用 EXTRA_COMPONENT_DIRS 指向本机 clone 下来的 espp 仓库的 components 目录。
cmake_minimum_required(VERSION 3.20)

set(ENV{IDF_COMPONENT_MANAGER} "0")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# ---- espp 组件库路径 ----------------------------------------------------------
# espp 是外部依赖(https://github.com/esp-cpp/espp),不在本仓库里,需要各自 clone。
# 路径不写死:优先读环境变量 ESPP_PATH,方便每台机器放在不同盘。
#
#   PowerShell : $env:ESPP_PATH = "D:\esp\espp"
#   cmd        : set ESPP_PATH=D:\esp\espp
#
# 没设环境变量时按下面的常见位置依次找,都找不到就直接报错并说清怎么办
# —— 比让 CMake 抛一堆 "component not found" 好排查。
if(DEFINED ENV{ESPP_PATH})
  set(_espp_root "$ENV{ESPP_PATH}")
else()
  foreach(_candidate
      "D:/esp/espp"
      "C:/esp/espp"
      "$ENV{HOME}/esp/espp"
      "$ENV{USERPROFILE}/esp/espp")
    if(EXISTS "${_candidate}/components")
      set(_espp_root "${_candidate}")
      break()
    endif()
  endforeach()
endif()

if(NOT _espp_root OR NOT EXISTS "${_espp_root}/components")
  message(FATAL_ERROR
    "\n找不到 espp 组件库。\n"
    "espp 是外部依赖,需要先 clone 到本机(不在本仓库内):\n"
    "    git clone --recursive https://github.com/esp-cpp/espp.git\n"
    "然后把它的路径告诉构建系统(指向仓库根目录,不是 components 子目录):\n"
    "    PowerShell : $env:ESPP_PATH = \"D:\\esp\\espp\"\n"
    "    cmd        : set ESPP_PATH=D:\\esp\\espp\n"
    "当前 ESPP_PATH = '$ENV{ESPP_PATH}'\n")
endif()

message(STATUS "espp components: ${_espp_root}/components")

set(EXTRA_COMPONENT_DIRS
  "${_espp_root}/components"
)

set(
  COMPONENTS
  "main esptool_py i2c as5600 drv2605 button adc display display_drivers spi filters task logger led format base_component esp_driver_gpio esp_driver_spi esp_driver_i2c esp_adc esp_driver_usb_serial_jtag lvgl nvs_flash"
  CACHE STRING
  "List of components to include"
  )

project(knob_firmware)

set(CMAKE_CXX_STANDARD 20)
