# modify 'target' and 'extraSrcCp' to add extension, besides, rules for building
# 'extraSrcCp' need to be defined manally

# write exptension's name here, should be identify with directory name
target := sample

# options, defines and includes for compiling, specially for this extension
$(target).optCc := $(EXT_CCOPTS)
$(target).optCp := $(EXT_CPOPTS)
$(target).def := $(EXT_DEFINES)
$(target).inc := \
	-I$(SRCDIR)/../server_tech_api/diameter_api/diameter_common_api/include \
	-I$(SRCDIR)/$(target)/inc \
	$(EXT_INCLUDES)

# the entire implementation of class need to be linked in, which is called in
# extension, including the others called by it
$(target).extraSrcCp := \
	$(SRCDIR)/../mboss_ts/dra/source/dict.cpp
$(target).extraObj := \
	$(foreach src,$($(target).extraSrcCp),$(OBJSDIR)/$(target)/$(notdir $(src)).o)

$(target).srcCcList := $(wildcard $(SRCDIR)/$(target)/src/*.c)
$(target).srcCpList := $(wildcard $(SRCDIR)/$(target)/src/*.cpp)
$(target).objCcList := $(foreach src,$($(target).srcCcList),$(call GetObjFromSrc,$(src)))
$(target).objCpList := $(foreach src,$($(target).srcCpList),$(call GetObjFromSrc,$(src)))
$(target).objList := $(foreach o,objCcList objCpList extraObj,$($(target).$(o)))

GetCmpOptCcFrmTgt = $(foreach sfx,optCc def inc,$($(call ParentName,$1).$(sfx)))
GetCmpOptCpFrmTgt = $(foreach sfx,optCp def inc,$($(call ParentName,$1).$(sfx)))

$(target).fdx: $(OUTPUTDIR)/$(target).fdx

$(OUTPUTDIR)/$(target).fdx: $($(target).objList)
	$(CXX) -shared $($(notdir $(basename $@)).optCc) -o $@ $^

# separate '.c' and '.cpp' to using diffrent compiling options, e.g. '-std=c++11' using for c++ exclusively
$($(target).objCcList):%.o : $$(call GetSrcFromObj,%)
	$(CC) -c $(call GetCmpOptCcFrmTgt,$@) -o $@ $<

$($(target).objCpList):%.o : $$(call GetSrcFromObj,%)
	$(CXX) -c $(call GetCmpOptCpFrmTgt,$@) -o $@ $<

$($(target).extraObj):%dict.cpp.o : $(SRCDIR)/../mboss_ts/dra/source/dict.cpp
	$(CXX) -c $(call GetCmpOptCpFrmTgt,$@) -o $@ $<

-include $($(target).objList:.o=.d)
