# Aerisys controller — main component.
# All .cpp/.c files anywhere under src/ are picked up automatically.
# Header search root is ../include; subfolders are addressed with prefix
# paths (e.g. #include "core/EspNowLink.h").

FILE(GLOB_RECURSE app_sources
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
)

idf_component_register(
    SRCS        ${app_sources}
    INCLUDE_DIRS "../include"
)
