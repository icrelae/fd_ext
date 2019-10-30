# write extensions/* in $(extList) to add extensions into compiling procedure;
# the name of config file's name should be same as extension's name, ie. derictory name;
# shared library file and config file will be output into $(OUTPUTDIR);
# the directory structure should be identify with 'sample', the 'sample.cfg' is optional;

CP = cp -rf
RM = rm -rf
MKDIR = mkdir -p

ParentName = $(lastword $(subst /, ,$(dir $(abspath $1))))
GetCfgSrc = $(SRCDIR)/$(notdir $(basename $1))/$(notdir $1)
GetSrcFromObj = $(SRCDIR)/$(call ParentName,$1)/src/$(notdir $1)
GetObjFromSrc = $(OBJSDIR)/$(call ParentName,$(dir $1))/$(notdir $1).o
CheckDefine = $(foreach 1,$1,$(call _CheckDefine,$1,$(strip $2)))
_CheckDefine = $(if $(value $1),,$(error Undefined $1$(if $2, ($2))))

$(call CheckDefine,VOB_ROOT API_BIN,(path for obj and src))

SRCDIR = $(VOB_ROOT)/extensions
OUTPUTDIR = $(API_BIN)/extensions
OBJSDIR = $(API_BIN)/obj_cc/extensions

# compiling options for all extensions
EXT_CCOPTS = -g -Wall -MMD -MP -fPIC -m32
EXT_CPOPTS = $(EXT_CCOPTS) -std=c++11
EXT_DEFINES = $(APP_OPT) -DEXTENSION
EXT_INCLUDES = \
			-I$(VOB_ROOT)/mboss_ts/dra/include

# add extensions here
extList = \
		  sample
cfgList = $(foreach ext,$(extList),$(OUTPUTDIR)/$(ext).cfg)
fdxList = $(foreach ext,$(extList),$(OUTPUTDIR)/$(ext).fdx)

extensions: $(OBJSDIR) $(OUTPUTDIR) $(cfgList) $(fdxList)

$(OBJSDIR):
	$(MKDIR) $(foreach ext,$(extList),$(OBJSDIR)/$(ext))

$(OUTPUTDIR):
	$(MKDIR) $(OUTPUTDIR)

# .cfg could be absent, therefore no prerequisites
$(cfgList):
	@[ -e $(call GetCfgSrc,$@) ] && $(CP) $(call GetCfgSrc,$@) $@ || \
		echo $(call GetCfgSrc,$@) not found!

.PHONY: cleanobj cleanext cleanall $(cfgList)

cleanobj:
	-$(RM) $(OBJSDIR)/*

cleanext:
	-$(RM) $(OUTPUTDIR)/*

cleanall: cleanobj cleanext
	-$(RM) $(OBJSDIR) $(OUTPUTDIR)

.SECONDEXPANSION:
include $(foreach src,$(extList),$(SRCDIR)/$(src)/makefile)
