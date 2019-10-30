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

#include <dlfcn.h>	/* We may use libtool's <ltdl.h> later for better portability.... */
#include <libgen.h>	/* for "basename" */
#include <cstring>
#include <cerrno>
#include <list>
#include <vector>
#include <string>
#include <sstream>

#include "cch_port.h"
#include "diameter_base.h"
#include "dau_common_def.h"
#include "extension.h"
#include "dict.h"

/* plugins management */

/* List of extensions to load, from the configuration parsing */
struct fd_ext_info {
	char 		*filename;	/* extension filename. must be a dynamic library with fd_ext_init symbol. */
	char 		*conffile;	/* optional configuration file name for the extension */
	void 		*handler;	/* object returned by dlopen() */
	const char 	**depends;	/* names of the other extensions this one depends on (if provided) */
	char		*ext_name;	/* points to the extension name, either inside depends, or basename(filename) */
	int		free_ext_name;	/* must be freed if it was malloc'd */
	void		(*fini)(void);	/* optional address of the fd_ext_fini callback */
};

/* list of extensions */
static std::list<struct fd_ext_info*> ext_list;

/* Add new extension */
int fd_ext_add( const char * filename, const char * conffile )
{
	UNUSED(error_message);
	UNUSED(avp_value_sizes);
	struct fd_ext_info * newExt;
	
	/* Check the filename is valid */
	if (NULL == filename || NULL == conffile)
		return -1;
	
	/* Create a new object in the list */
	newExt = new(struct fd_ext_info);
	if (NULL == newExt)
		return -2;
	memset(newExt, 0, sizeof(struct fd_ext_info));
	newExt->filename = strndup(filename, 128);
	newExt->conffile = strndup(conffile, 128);
	if (NULL == newExt->filename || NULL == newExt->conffile)
		return -3;
	ext_list.push_back(newExt);
	//TRACE_DEBUG (FULL, "Extension %s added to the list.", filename);
	return 0;
}

/* Check the dependencies. The object must have been dlopened already. */
static int check_dependencies(struct fd_ext_info * ext)
{
	int i = 1;
	
	/* Attempt to resolve the dependency array */
	*((void**)&ext->depends) = dlsym( ext->handler, "fd_ext_depends" );
	if (!ext->depends) {
		/* Duplicate the filename */
		char * tmp = strdup(ext->filename);
		ext->ext_name = strdup(basename(tmp));
		delete(tmp);
		ext->free_ext_name = 1;
		//TRACE_DEBUG(FULL, "Old extension's [%s] API: missing dependencies (ignored)", ext->ext_name);
		return 0;
	}
	
	ext->ext_name = (char *)ext->depends[0];
	
	//TRACE_DEBUG(FULL, "Checking dependencies for '%s'...", ext->ext_name);
	
	while (ext->depends[i]) {
		std::list<struct fd_ext_info*>::const_iterator li;
		for (li = ext_list.begin(); li != ext_list.end() && *li != ext; ++li)
		{
			struct fd_ext_info * e = *li;
			if (!strcasecmp(e->ext_name, ext->depends[i])) {
				/* the dependency was already loaded */
				break;
			}
		}
		
		if (*li == ext) {
			/* the dependency was not found */
			//LOG_F("Error: extension [%s] depends on [%s] which was not loaded first. Please fix your configuration file.",
			//	ext->ext_name, ext->depends[i]);
			return ESRCH;
		}
		
		i++;
	}

	/* All dependencies resolved successfully */
	return 0;
}

/* Load all extensions in the list */
int fd_ext_load()
{
	int ret;
	int (*fd_ext_init)(int, int, fd_ext_arg *) = NULL;
	std::list<struct fd_ext_info*>::iterator li;
	dictionary *pDictionary = dictionary::getInstance();
	
	/* Loop on all extensions */
	for (li = ext_list.begin(); li != ext_list.end(); ++li)
	{
		struct fd_ext_info * ext = *li;
		//LOG_D( "Loading : %s", ext->filename);
		
		/* Load the extension */
#ifndef DEBUG
		ext->handler = dlopen(ext->filename, RTLD_LAZY | RTLD_GLOBAL);
#else /* DEBUG */
		/* We resolve symbols immediatly so it's easier to find problems in ABI */
		ext->handler = dlopen(ext->filename, RTLD_NOW | RTLD_GLOBAL);
#endif /* DEBUG */
		if (ext->handler == NULL) {
			/* An error occured */
			//LOG_F("Loading of extension %s failed: %s", ext->filename, dlerror());
			ext->handler = dlopen(ext->filename, RTLD_LAZY | RTLD_GLOBAL);
			if (ext->handler) {
				if (check_dependencies(ext)) {
					//LOG_F("In addition, not all declared dependencies are satisfied (Internal Error!)");
				}
				dlclose(ext->handler);
				ext->handler = NULL;
			}
			return EINVAL;
		}
		
		/* Check if declared dependencies are satisfied. */
		if (check_dependencies(ext)) {
			dlclose(ext->handler);
			ext->handler = NULL;
			return EINVAL;
		}
		
		/* Resolve the entry point of the extension */
		fd_ext_init = ( int (*) (int, int, fd_ext_arg *) )dlsym( ext->handler, "fd_ext_init" );
		
		if (fd_ext_init == NULL) {
			/* An error occured */
			//TRACE_ERROR("Unable to resolve symbol 'fd_ext_init' for extension %s: %s", ext->filename, dlerror());
			dlclose(ext->handler);
			ext->handler = NULL;
			return EINVAL;
		}
		
		/* Resolve the exit point of the extension, which is optional for extensions */
		ext->fini = ( void (*) (void) )dlsym( ext->handler, "fd_ext_fini" );
		
		if (ext->fini == NULL) {
			/* Not provided */
			//TRACE_DEBUG (FULL, "Extension [%s] has no fd_ext_fini function.", ext->filename);
		} else {
			/* Provided */
			//TRACE_DEBUG (FULL, "Extension [%s] fd_ext_fini has been resolved successfully.", ext->filename);
		}
		
		/* Now call the entry point to initialize the extension */
		struct fd_ext_arg args = {};
		args.conffile = ext->conffile;
		args.dict = static_cast<void*>(pDictionary);
		ret = (*fd_ext_init)( 1, 2, &args );
		if (ret != 0) {
			/* The extension was unable to load cleanly */
			//TRACE_ERROR("Extension %s returned an error during initialization: %s", ext->filename, strerror(ret));
			return ret;
		}
		
		/* Proceed to the next extension */
	}

	//LOG_N("All extensions loaded.");
	
	/* We have finished. */
	return 0;
}

/* Now unload the extensions and free the memory */
int fd_ext_term( void )
{
	/* Loop on all extensions, in FIFO order */
	while (!ext_list.empty())
	{
		std::list<struct fd_ext_info*>::iterator li = ext_list.begin();
		struct fd_ext_info * ext = *li;
	
		/* Unlink this element from the list */
		ext_list.erase(li);
		
		/* Call the exit point of the extension, if it was resolved */
		if (ext->fini != NULL) {
			//TRACE_DEBUG (FULL, "Calling [%s]->fd_ext_fini function.", ext->ext_name ?: ext->filename);
			(*ext->fini)();
		}
		
#ifndef SKIP_DLCLOSE
		/* Now unload the extension */
		if (ext->handler) {
			//TRACE_DEBUG (FULL, "Unloading %s", ext->ext_name ?: ext->filename);
			if ( dlclose(ext->handler) != 0 ) {
				//TRACE_DEBUG (INFO, "Unloading [%s] failed : %s", ext->ext_name ?: ext->filename, dlerror());
			}
		}
#endif /* SKIP_DLCLOSE */
		
		/* Free the object and continue */
		if (ext->free_ext_name)
			free(ext->ext_name);
		free(ext->filename);
		free(ext->conffile);
		delete ext;
	}
	
	/* We always return 0 since we would not handle an error anyway... */
	return 0;
}

static std::vector<std::string> split(const std::string &s, char delim)
{
	std::string item;
	std::stringstream ss(s);
	std::vector<std::string> elems;
	while (getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

static std::string &strip(std::string &s)
{
	std::string::iterator it = s.begin();
	while (it != s.end()) {
		if (std::isspace(*it))
			it = s.erase(it);
		else
			++it;
	}
	return s;
}

int fd_ext_initialize(void *pParam)
{
	char buf[FILE_NAME_LENGTH] = {0};
	std::string pHome(static_cast<char*>(pParam));
	std::string extHome = pHome + "lib/extensions/";
	std::string cfgFile = pHome + "cfg/extensions.cfg";

	GetPrivateProfileString("Extension", "ExtensionList", "", buf, sizeof(buf)-1, cfgFile.c_str());
	std::string extName(buf);
	strip(extName);
	std::vector<std::string> vecExtName = split(extName, ',');
	for (auto it = vecExtName.cbegin(); it != vecExtName.cend(); ++it) {
		std::string fdxPath = extHome + *it + ".fdx";
		std::string cfgPath = extHome + *it + ".cfg";
		if (fd_ext_add(fdxPath.c_str(), cfgPath.c_str()))
			return RTN_FAIL;
	}
	
	return RTN_SUCCESS;
}
