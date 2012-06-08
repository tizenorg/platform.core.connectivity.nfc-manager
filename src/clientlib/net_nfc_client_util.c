/*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at

  *     http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */

#include <stdlib.h>
#include <string.h>

#include "net_nfc_client_util_private.h"

void __net_nfc_client_util_free_mem(void** mem, char * filename, unsigned int line)
{
	if (mem == NULL || *mem == NULL)
	{
		return;
	}

	free(*mem);

	*mem = NULL;
}

void __net_nfc_client_util_alloc_mem(void** mem, int size, char * filename, unsigned int line)
{
	if (mem == NULL || size <= 0)
	{
		return;
	}

	*mem = malloc(size);

	if (*mem != NULL)
	{
		memset(*mem, 0x0, size);
	}
}

