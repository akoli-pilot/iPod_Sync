cmake_minimum_required(VERSION 3.18)
project(ipod_native LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# cmake-js provides these
if(NOT DEFINED CMAKE_JS_INC)
  message(FATAL_ERROR "This project is intended to be built via cmake-js.")
endif()

find_package(PkgConfig)
option(USE_LIBIMOBILEDEVICE "Enable libimobiledevice for mobile devices" ON)
option(USE_LIBUDEV "Enable libudev for disk-mode detection" ON)

if(PkgConfig AND USE_LIBIMOBILEDEVICE)
  pkg_check_modules(LIBIMOBILEDEVICE libimobiledevice-1.0)
  pkg_check_modules(LIBPLIST libplist-2.0)
endif()

if(PkgConfig AND USE_LIBUDEV)
  pkg_check_modules(LIBUDEV libudev)
endif()

add_library(${PROJECT_NAME} SHARED addon.cpp ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_JS_INC}
  ${CMAKE_CURRENT_SOURCE_DIR}/node_modules/node-addon-api
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_JS_LIB})

target_compile_definitions(${PROJECT_NAME} PRIVATE NAPI_CPP_EXCEPTIONS)

if(LIBIMOBILEDEVICE_FOUND AND LIBPLIST_FOUND)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_LIBIMOBILEDEVICE)
  target_include_directories(${PROJECT_NAME} PRIVATE ${LIBIMOBILEDEVICE_INCLUDE_DIRS} ${LIBPLIST_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBIMOBILEDEVICE_LIBRARIES} ${LIBPLIST_LIBRARIES})
endif()

if(LIBUDEV_FOUND)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_LIBUDEV)
  target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUDEV_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBUDEV_LIBRARIES})
endif()
