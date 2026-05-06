set(PLATFORM_SOURCES 3rdparty/WinCommander.cpp src/sys/windows/guihelper.cpp src/sys/windows/MiniDump.cpp src/sys/windows/eventHandler.cpp src/sys/windows/WinVersion.cpp src/sys/windows/AutoRun.cpp)
set(PLATFORM_LIBRARIES wininet wsock32 ws2_32 user32 rasapi32 iphlpapi ntdll wbemuuid)

include(cmake/windows/generate_product_version.cmake)

set(THRONE_PRODUCT_VERSION "$ENV{INPUT_VERSION}")
if ("${THRONE_PRODUCT_VERSION}" STREQUAL "" OR "${THRONE_PRODUCT_VERSION}" STREQUAL "manual")
    set(THRONE_PRODUCT_VERSION "0.1.0")
endif()
string(REGEX REPLACE "^v" "" THRONE_PRODUCT_VERSION "${THRONE_PRODUCT_VERSION}")
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(\\.([0-9]+))?" THRONE_PRODUCT_VERSION_MATCH "${THRONE_PRODUCT_VERSION}")
if (THRONE_PRODUCT_VERSION_MATCH)
    set(THRONE_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(THRONE_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(THRONE_VERSION_PATCH "${CMAKE_MATCH_3}")
    if ("${CMAKE_MATCH_5}" STREQUAL "")
        set(THRONE_VERSION_REVISION 0)
    else()
        set(THRONE_VERSION_REVISION "${CMAKE_MATCH_5}")
    endif()
else()
    set(THRONE_VERSION_MAJOR 0)
    set(THRONE_VERSION_MINOR 1)
    set(THRONE_VERSION_PATCH 0)
    set(THRONE_VERSION_REVISION 0)
endif()

generate_product_version(
        QV2RAY_RC
        ICON "${CMAKE_SOURCE_DIR}/res/Throne.ico"
        NAME "Throne-Mod"
        BUNDLE "Throne-Mod"
        COMPANY_NAME "Throne-Mod"
        COMPANY_COPYRIGHT "Throne-Mod"
        FILE_DESCRIPTION "Throne-Mod"
        COMMENTS "Throne-Mod v${THRONE_PRODUCT_VERSION}"
        VERSION_MAJOR ${THRONE_VERSION_MAJOR}
        VERSION_MINOR ${THRONE_VERSION_MINOR}
        VERSION_PATCH ${THRONE_VERSION_PATCH}
        VERSION_REVISION ${THRONE_VERSION_REVISION}
)
add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)
set(GUI_TYPE WIN32)
if (MSVC)
    add_compile_options("/utf-8")
    add_definitions(-D_WIN32_WINNT=0x600 -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)
endif ()
