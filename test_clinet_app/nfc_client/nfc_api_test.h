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

#ifndef __NET_NFC_TEST_H__
#define __NET_NFC_TEST_H__

typedef enum {
	NET_NFC_TEST_NOT_YET,
	NET_NFC_TEST_OK,
	NET_NFC_TEST_FAIL,
}net_nfc_test_result_e;


#define LOG_COLOR_RED 		"\033[0;31m"
#define LOG_COLOR_GREEN 	"\033[0;32m"
#define LOG_COLOR_BROWN 	"\033[0;33m"
#define LOG_COLOR_BLUE 		"\033[0;34m"
#define LOG_COLOR_PURPLE 	"\033[0;35m"
#define LOG_COLOR_CYAN 		"\033[0;36m"
#define LOG_COLOR_LIGHTBLUE "\033[0;37m"
#define LOG_COLOR_END		"\033[0;m"

#define PRINT_INSTRUCT(format,args...) \
do {\
	printf(LOG_COLOR_BLUE""format""LOG_COLOR_END"\n", ##args);\
}while(0);

#define PRINT_RESULT_FAIL(format,args...) \
do {\
	printf(LOG_COLOR_RED""format""LOG_COLOR_END"\n", ##args);\
}while(0);

#define PRINT_RESULT_SUCCESS(format,args...) \
do {\
	printf(LOG_COLOR_GREEN""format""LOG_COLOR_END"\n", ##args);\
}while(0);

#define PRINT_INFO(format,args...) \
do {\
	printf(format"\n", ##args);\
}while(0);


#define CHECK_RESULT(X)\
	do{\
		if(X!=NET_NFC_OK){\
			PRINT_RESULT_FAIL("FILE:%s, LINE:%d, RESULT:%d",__FILE__,__LINE__,X);\
			return NET_NFC_TEST_FAIL;\
		}\
	}while(0)


#define CHECK_ASSULT(X)\
	do{\
		if(!(X)){\
			PRINT_RESULT_FAIL("FILE:%s, LINE:%d, RESULT:%d",__FILE__,__LINE__,X);\
			return NET_NFC_TEST_FAIL;\
		}\
	}while(0)

/* struct defines */
typedef int nfcTestStartFn_t(uint8_t testNumber,void* arg_ptr2);

typedef struct
{
  char*   				testName;
  nfcTestStartFn_t*  	testFn;
  uint8_t        		testResult;
} nfcTestType;




#endif

