/*********************************************************************************************************
* Software License Agreement (BSD License)                                                               *
* Author: Sebastien Decugis <sdecugis@freediameter.net>							 *
*													 *
* Copyright (c) 2013, WIDE Project and NICT								 *
* All rights reserved.											 *
* 													 *
* Redistribution and use of this software in source and binary forms, with or without modification, are  *
* permitted provided that the following conditions are met:						 *
* 													 *
* * Redistributions of source code must retain the above 						 *
*   copyright notice, this list of conditions and the 							 *
*   following disclaimer.										 *
*    													 *
* * Redistributions in binary form must reproduce the above 						 *
*   copyright notice, this list of conditions and the 							 *
*   following disclaimer in the documentation and/or other						 *
*   materials provided with the distribution.								 *
* 													 *
* * Neither the name of the WIDE Project or NICT nor the 						 *
*   names of its contributors may be used to endorse or 						 *
*   promote products derived from this software without 						 *
*   specific prior written permission of WIDE Project and 						 *
*   NICT.												 *
* 													 *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED *
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A *
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 	 *
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 	 *
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR *
* TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF   *
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.								 *
*********************************************************************************************************/

/* Sample file demonstrating how to write some C++ code */

#include <iostream>
#include "dict.h"

extern "C" void addavp(void *args) {
	UNUSED(error_message);
	UNUSED(avp_value_sizes);
	/*
	 * symbols of extension and main procedure belong to different code segment;
	 * therefore the global pointer of main procedure need to be passed in,
	 * intead of access through class static member;
	 */
	dictionary *pDictionary = static_cast<dictionary*>(args);
	printf("Let's create that 'Example-AVP'...\n");
	{
		struct dict_object * origin_host_avp = NULL;
		struct dict_object * session_id_avp = NULL;
		struct dict_object * example_avp_avp = NULL;
		struct dict_rule_data rule_data = { NULL, RULE_REQUIRED, 0, -1, 1 };
		struct dict_avp_data example_avp_data = { 999999, 0, (char*)"Example-AVP", AVP_FLAG_VENDOR , 0, AVP_TYPE_GROUPED };

		pDictionary->fd_dict_search (DICT_AVP, AVP_BY_NAME, "Origin-Host", &origin_host_avp, ENOENT);
		pDictionary->fd_dict_search (DICT_AVP, AVP_BY_NAME, "Session-Id", &session_id_avp, ENOENT);

		pDictionary->fd_dict_new ( DICT_AVP, &example_avp_data , NULL, &example_avp_avp );

		rule_data.rule_avp = origin_host_avp;
		rule_data.rule_min = 1;
		rule_data.rule_max = 1;
		pDictionary->fd_dict_new ( DICT_RULE, &rule_data, example_avp_avp, NULL );

		rule_data.rule_avp = session_id_avp;
		rule_data.rule_min = 1;
		rule_data.rule_max = -1;
		pDictionary->fd_dict_new ( DICT_RULE, &rule_data, example_avp_avp, NULL );

	}
	printf("'Example-AVP' created without error\n");
	struct dict_object *ext = NULL;
	pDictionary->fd_dict_search( DICT_AVP, AVP_BY_NAME, (char*)"Example-AVP", &ext, ENOENT);
	printf("dict: %p, Example-AVP: %p\n", pDictionary, ext);
}
