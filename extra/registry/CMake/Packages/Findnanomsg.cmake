# Tries to locate nanomsg headers and libraries.
#
# Usage:
#
#     find_package(nanomsg)
#
#     NANOMSG_ROOT_DIR may be defined beforehand to hint at install location.
#
# Variables defined after calling:
#
#     NANOMSG_FOUND       - whether a nanomsg installation is located
#     NANOMSG_INCLUDE_DIR - path to nanomsg headers
#     NANOMSG_LIBRARY     - path of nanomsg library

find_path(NANOMSG_ROOT_DIR
    NAMES include/nanomsg/nn.h
)

find_path(NANOMSG_INCLUDE_DIR
    NAMES nanomsg/nn.h
    HINTS ${NANOMSG_ROOT_DIR}/include
)

find_library(NANOMSG_LIBRARY
    NAMES nanomsg
    HINTS ${NANOMSG_ROOT_DIR}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(nanomsg DEFAULT_MSG
    NANOMSG_INCLUDE_DIR
    NANOMSG_LIBRARY
)

mark_as_advanced(
    NANOMSG_ROOT_DIR
    NANOMSG_INCLUDE_DIR
    NANOMSG_LIBRARY
)