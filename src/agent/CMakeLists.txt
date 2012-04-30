CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(nfc C)

SET(AGENT_DAEMON "nfc-agent")

SET(AGENT_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/net_nfc_agent.c
	${CMAKE_CURRENT_SOURCE_DIR}/net_nfc_dbus_agent_obj.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../commonlib/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../manager/include)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")

INCLUDE(FindPkgConfig)

MESSAGE("CHECK MODULE in MANAGER ${agent_pkges_LDFLAGS}")

pkg_check_modules(agent_pkges REQUIRED glib-2.0 gobject-2.0 vconf dlog dbus-glib-1)

FOREACH(flag ${agent_pkges_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)


# this for NFC flag

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -pipe -fomit-frame-pointer -Wall -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing")

SET(ARM_CFALGS "${ARM_CFLAGS} -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -msoft-float -Uarm -fno-common -fpic")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror-implicit-function-declaration")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")

FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" MATCHES "^arm.*")
	ADD_DEFINITIONS("-DTARGET")
	MESSAGE("add -DTARGET")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARM_CFLAGS}")
ENDIF()

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DSLP_DEBUG")
ADD_DEFINITIONS("-fpie")

#ADD_DEFINITIONS("-DSECURITY_SERVER")

SET (DBUS_AUTO_ACTIVE 
	${CMAKE_CURRENT_SOURCE_DIR}/resource/com.samsung.slp.nfc.agent.service
)

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

ADD_EXECUTABLE(${AGENT_DAEMON} ${AGENT_SRCS})

TARGET_LINK_LIBRARIES(${AGENT_DAEMON} "-pie")
TARGET_LINK_LIBRARIES(${AGENT_DAEMON} ${agent_pkges_LDFLAGS})

ADD_CUSTOM_TARGET(dbus_agent_glue_private.h
		COMMAND dbus-binding-tool --prefix=dbus_agent ${CMAKE_CURRENT_SOURCE_DIR}/resource/dbus_agent.xml 
		--mode=glib-server --output=${CMAKE_CURRENT_SOURCE_DIR}/include/dbus_agent_glue_private.h
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resource/dbus_agent.xml
)
ADD_DEPENDENCIES(${AGENT_DAEMON} dbus_agent_glue_private.h)

ADD_CUSTOM_TARGET(dbus_agent_binding_private.h
		COMMAND dbus-binding-tool --prefix=dbus_agent ${CMAKE_CURRENT_SOURCE_DIR}/resource/dbus_agent.xml 
		--mode=glib-client --output=${CMAKE_CURRENT_SOURCE_DIR}/include/dbus_agent_binding_private.h
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resource/dbus_agent.xml
)
ADD_DEPENDENCIES(${AGENT_DAEMON} dbus_agent_binding_private.h)

INSTALL(TARGETS ${AGENT_DAEMON} DESTINATION bin)
INSTALL(FILES ${DBUS_AUTO_ACTIVE} DESTINATION share/dbus-1/services/)

