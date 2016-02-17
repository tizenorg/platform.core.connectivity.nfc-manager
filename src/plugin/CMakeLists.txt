CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET (this_target cardemulation_plugin)
SET (this_target_2 useese_plugin)

SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/cmake_build_tmp/output)
include_directories(${CMAKE_SOURCE_DIR}/src/commonlib/include)
include_directories(${CMAKE_SOURCE_DIR}/src/clientlib/include)
#include_directories(${CMAKE_SOURCE_DIR}/src/clientlib)

#INCLUDE(FindPkgConfig)
#pkg_check_modules(pkg REQUIRED glib-2.0 dlog libxml-2.0)
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs_plugin REQUIRED
	glib-2.0
	pkgmgr-info
	libxml-2.0
	dlog
	syspopup-caller
)

FOREACH(flag ${pkgs_plugin_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET (${this_target}_SOURCE_FILES
    pkgmgr_plugin_cardemulation.c
)
SET (${this_target_2}_SOURCE_FILES
    pkgmgr_plugin_useese.c
)

ADD_DEFINITIONS("-DDLOG_ERROR_ENABLED")
ADD_DEFINITIONS("-DLOG_TAG=\"CARDEMULATION_PLUGIN\"")
SET(EXTRA_CFLAGS  "${EXTRA_CFLAGS} -Wall" )

SET(CMAKE_C_FLAGS_PROFILING    " -g -pg")
SET(CMAKE_CXX_FLAGS_PROFILING  " -std=c++0x -g -pg")
SET(CMAKE_C_FLAGS_DEBUG        " -g")
SET(CMAKE_CXX_FLAGS_DEBUG      " -std=c++0x -g")
SET(CMAKE_C_FLAGS_RELEASE      " -g")
SET(CMAKE_CXX_FLAGS_RELEASE    " -std=c++0x -g")
SET(CMAKE_C_FLAGS_CCOV         " -g --coverage")
SET(CMAKE_CXX_FLAGS_CCOV       " -std=c++0x -g --coverage")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -std=gnu99")

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -pipe -fomit-frame-pointer -Wall -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -Wl,-zdefs")

SET(ARM_CFLAGS "${ARM_CLAGS} -mapcs -mno-sched-prolog -mabi=aapcs-linux -Uarm -fno-common -fpic")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror-implicit-function-declaration")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")


## Create Library
ADD_LIBRARY (${this_target} SHARED ${${this_target}_SOURCE_FILES} )
ADD_LIBRARY (${this_target_2} SHARED ${${this_target_2}_SOURCE_FILES} )
## SET LINKER FLAGS
#SET(CMAKE_SHARED_LINKER_FLAGS -Wl,--no-undefined)

TARGET_LINK_LIBRARIES(${this_target} ${pkgs_plugin_LDFLAGS} nfc)
TARGET_LINK_LIBRARIES(${this_target_2} ${pkgs_plugin_LDFLAGS} nfc)

INSTALL(TARGETS ${this_target} DESTINATION "/usr/etc/package-manager/parserlib/metadata")
INSTALL(TARGETS ${this_target_2} DESTINATION "/usr/etc/package-manager/parserlib/metadata")

