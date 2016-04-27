CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(nfc-common-lib C)

SET(NFC_COMMON_LIB "nfc-common-lib")
SET(VERSION_MAJOR 1)
SET(VERSION ${VERSION_MAJOR}.0.0)

FIND_PROGRAM(GDBUS_CODEGEN NAMES gdbus-codegen)
EXEC_PROGRAM(${GDBUS_CODEGEN} ARGS
		" \\
		--generate-c-code ${CMAKE_CURRENT_SOURCE_DIR}/net_nfc_gdbus \\
		--c-namespace NetNfcGDbus\\
		--interface-prefix org.tizen.NetNfcService. \\
		${CMAKE_CURRENT_SOURCE_DIR}/net_nfc.xml \\
		")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} COMMON_LIB_SRCS)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")

INCLUDE(FindPkgConfig)
pkg_check_modules(commonlib_pkges REQUIRED
		glib-2.0 gio-2.0 gio-unix-2.0 dlog bluetooth-api openssl aul pkgmgr-info libsystemd-daemon)

FOREACH(flag ${commonlib_pkges_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

MESSAGE("CHECK MODULE in COMMON_LIB ${commonlib_pkges_LDFLAGS}")

# this for NFC flag

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -pipe -fomit-frame-pointer -Wall -Wno-trigraphs -Werror-implicit-function-declaration  -fno-strict-aliasing -Wl,-zdefs")

SET(ARM_CFLAGS "${ARM_CLAGS} -mapcs -mno-sched-prolog -mabi=aapcs-linux -Uarm -fno-common -fpic")

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

ADD_LIBRARY(${NFC_COMMON_LIB} SHARED ${COMMON_LIB_SRCS})
SET_TARGET_PROPERTIES(${NFC_COMMON_LIB} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${NFC_COMMON_LIB} PROPERTIES VERSION ${VERSION})

TARGET_LINK_LIBRARIES(${NFC_COMMON_LIB} ${commonlib_pkges_LDFLAGS})

SET(COMMON_LIB_HEADER
	include/net_nfc_typedef.h
	include/net_nfc_util_internal.h
	include/net_nfc_oem_controller.h
	include/net_nfc_typedef_internal.h
	include/net_nfc_util_ndef_message.h
	include/net_nfc_util_handover.h
	include/net_nfc_util_ndef_record.h
	include/net_nfc_util_sign_record.h
	include/net_nfc_debug_internal.h
	include/net_nfc_util_gdbus_internal.h
	net_nfc_gdbus.h
	include/net_nfc_util_defines.h
	include/net_nfc_util_hce.h
)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${NFC_COMMON_LIB}.pc.in ${CMAKE_CURRENT_SOURCE_DIR}/${NFC_COMMON_LIB}.pc)

IF (TIZEN_ARCH_64)
	INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${NFC_COMMON_LIB}.pc DESTINATION /usr/lib64/pkgconfig)
ELSE (TIZEN_ARCH_64)
	INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${NFC_COMMON_LIB}.pc DESTINATION /usr/lib/pkgconfig)
ENDIF(TIZEN_ARCH_64)

IF (TIZEN_ARCH_64)
        INSTALL(TARGETS ${NFC_COMMON_LIB} DESTINATION lib64)
ELSE (TIZEN_ARCH_64)
        INSTALL(TARGETS ${NFC_COMMON_LIB} DESTINATION lib)
ENDIF(TIZEN_ARCH_64)

FOREACH(hfile ${COMMON_LIB_HEADER})
        INSTALL(FILES ${hfile} DESTINATION include)
ENDFOREACH(hfile)

