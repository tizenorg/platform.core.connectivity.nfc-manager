/*
 * Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * This file is part of nfc-manager
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with SAMSUNG ELECTRONICS.
 *
 * SAMSUNG make no representations or warranties about the suitability
 * of the software, either express or implied, including but not limited
 * to the implied warranties of merchantability, fitness for a particular
 * purpose, or non-infringement. SAMSUNG shall not be liable for any
 * damages suffered by licensee as a result of using, modifying or
 * distributing this software or its derivatives.
 *
 */

#ifndef __NET_NFC_DEBUG_PRIVATE_H__
#define __NET_NFC_DEBUG_PRIVATE_H__

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <libgen.h>

// below define should define before dlog.h
#define LOG_TAG "NET_NFC_MANAGER"

#include <dlog.h>

#define LOG_COLOR_RED 		"\033[0;31m"
#define LOG_COLOR_GREEN 	"\033[0;32m"
#define LOG_COLOR_BROWN 	"\033[0;33m"
#define LOG_COLOR_BLUE 		"\033[0;34m"
#define LOG_COLOR_PURPLE 	"\033[0;35m"
#define LOG_COLOR_CYAN 		"\033[0;36m"
#define LOG_COLOR_LIGHTBLUE "\033[0;37m"
#define LOG_COLOR_END		"\033[0;m"


#define DEBUG_MSG_PRINT_BUFFER(buffer,length) \
do {\
	LOGD(LOG_COLOR_GREEN"[file = %s : method = %s : line = %d]"LOG_COLOR_END, basename(__FILE__), __func__, __LINE__); \
	int i = 0;\
	LOGD(LOG_COLOR_BLUE"BUFFER => \n"LOG_COLOR_END);\
	for(; i < length; i++)\
	{\
		LOGD(LOG_COLOR_BLUE" [0x%x] "LOG_COLOR_END,buffer[i]);\
	}\
	LOGD(LOG_COLOR_BLUE"\n\n"LOG_COLOR_END);\
}while(0);

#define DEBUG_MSG_PRINT_BUFFER_CHAR(buffer,length) \
do {\
	LOGD(LOG_COLOR_GREEN"[file = %s : method = %s : line = %d]"LOG_COLOR_END, basename(__FILE__), __func__, __LINE__); \
	int i = 0;\
	LOGD(LOG_COLOR_BLUE"BUFFER => \n"LOG_COLOR_END);\
	for(; i < length; i++)\
	{\
		LOGD(LOG_COLOR_BLUE" [%c] "LOG_COLOR_END,buffer[i]);\
	}\
	LOGD(LOG_COLOR_BLUE"\n\n"LOG_COLOR_END);\
}while(0);

#define DEBUG_MSG(format,args...) \
do {\
	LOGD(LOG_COLOR_BROWN"[file = %s : method = %s : line = %d]"LOG_COLOR_END, basename(__FILE__), __func__, __LINE__); \
	LOGD(LOG_COLOR_BROWN""format""LOG_COLOR_END, ##args);\
}while(0);

#define DEBUG_SERVER_MSG(format,args...) \
do {\
	LOGD(LOG_COLOR_PURPLE"[file = %s : method = %s : line = %d]"LOG_COLOR_END, basename(__FILE__), __func__, __LINE__); \
	LOGD(LOG_COLOR_PURPLE""format""LOG_COLOR_END, ##args);\
}while(0);

#define DEBUG_CLIENT_MSG(format,args...) \
do {\
	LOGD(LOG_COLOR_GREEN"[file = %s : method = %s : line = %d]"LOG_COLOR_END, basename(__FILE__), __func__, __LINE__); \
	LOGD(LOG_COLOR_GREEN""format""LOG_COLOR_END, ##args);\
}while(0);

#define DEBUG_ERR_MSG(format,args...) \
do {\
	LOGD(LOG_COLOR_RED"[ERR :: file = %s : method = %s : line = %d]"LOG_COLOR_END, basename(__FILE__), __func__, __LINE__); \
	LOGD(LOG_COLOR_RED""format""LOG_COLOR_END, ##args);\
}while(0);

#define PROFILING(str) \
do{ \
	struct timeval mytime;\
	char buf[128]; = {0};\
	memset(buf, 0x00, 128);\
	gettimeofday(&mytime, NULL);\
	char time_string[128] = {0,};\
	sprintf(time_string, "%d.%4d", mytime.tv_sec, mytime.tv_usec);\
	LOGD(str); \
	LOGD("\t time = [%s] \n", time_string);\
}while(0);

#endif
