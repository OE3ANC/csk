if (EXISTS "${CSK_DIR}/VERSION")
    file(READ "${CSK_DIR}/VERSION" VERSION_FROM_FILE)
else()
    message(FATAL_ERROR "file ${CSK_DIR}/VERSION not found")
endif()


file(READ "${CSK_DIR}/VERSION" VERSION_CONTENTS)

# 使用正则表达式匹配版本号
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)" VERSION_REGEX "${VERSION_CONTENTS}")

if(NOT VERSION_REGEX)
message(FATAL_ERROR "CSK SDK Version format error")
endif()

# 提取版本号的各个部分
set(CSK_SDK_VERSION_MAJOR ${CMAKE_MATCH_1})
set(CSK_SDK_VERSION_MINOR ${CMAKE_MATCH_2})
set(CSK_SDK_VERSION_PATCH ${CMAKE_MATCH_3})
set(CSK_SDK_VERSION_BUILD ${CMAKE_MATCH_4})

set(CSK_SDK_VERSION_STRING ${VERSION_FROM_FILE})

find_package(Git QUIET)
if(GIT_FOUND)
execute_process(
    COMMAND git describe --always --dirty --tag --abbrev=8
    OUTPUT_VARIABLE                  CSK_SDK_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE                   stderr
    RESULT_VARIABLE                  return_code
    WORKING_DIRECTORY                ${CMAKE_CURRENT_SOURCE_DIR}
    )
    if(return_code)
        message(STATUS "git describe failed: ${stderr}")
    elseif(NOT "${stderr}" STREQUAL "")
        message(STATUS "git describe warned: ${stderr}")
    endif()
endif()

message(STATUS "Listenai CSK Version: ${VERSION_FROM_FILE}")
message(STATUS "Listenai CSK Commit:${CSK_SDK_GIT_COMMIT}")

configure_file(${CSK_DIR}/version.h.in ${ZEPHYR_BINARY_DIR}/include/generated/csk_sdk_version.h)