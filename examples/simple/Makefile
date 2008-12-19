#########################################################
##                                                     ##
##       Generic makefile for simple projects.         ## 
##                                                     ##
##  Makes one executable, named with the EXE variable. ##
##   All source goes in a flat directory, named ./src  ##
##                                                     ##
#########################################################

#name of the executable
EXE = simple

#include directories
INCLUDE_DIRS = -I../../include

#compiler flags
COMPILE_FLAGS = -O2

#lib directories
LIB_DIRS = -L../.. 

#linker flags
LINK_FLAGS = -lz -ltourtre

#preprocessor defines
DEFINES = -DUSE_ZLIB

#source suffixes to scan for
SOURCE_SUFFIXES = cpp cc

#directory to scan
SOURCE_DIR = src

#temporaries dir (object files, moc files, etc)
OBJ_DIR = obj



#targets
all: $(EXE)

clean: 
	-rm -f obj/*
	-rm -f $(EXE)

remake: clean $(EXE)



#standard make vars
CXX = g++
CC = gcc
CFLAGS = $(INCLUDE_DIRS) $(COMPILE_FLAGS)
CXXFLAGS = $(INCLUDE_DIRS) $(COMPILE_FLAGS)
CPPFLAGS = $(DEFINES) $(INCLUDE_DIRS)
LDFLAGS = $(LIB_DIRS) $(LINK_FLAGS)






#####################################################
## everything beyond this point should be automatic,
## assuming all your source files are in ./src .
## For each file in ./src that matches a suffix in 
## SOURCE_SUFFIXES, .o and .d files with the same name
## will be made in ./obj, which must exist. The .o files
## will be used to build EXE, and the .d files will be
## included in the makefile. The .d files contain targets
## for themselves so dependencies should be self-updating

OBJ_FILES = $(foreach suffix,$(SOURCE_SUFFIXES),$(addprefix $(OBJ_DIR)/,$(notdir $(subst .$(suffix),.o,$(wildcard $(SOURCE_DIR)/*.$(suffix))))))
DEP_FILES = $(foreach suffix,$(SOURCE_SUFFIXES),$(addprefix $(OBJ_DIR)/,$(notdir $(subst .$(suffix),.d,$(wildcard $(SOURCE_DIR)/*.$(suffix))))))

$(EXE): $(OBJ_FILES) $(DEP_FILES)
	$(CXX) -o $(EXE) $(filter $(OBJ_DIR)/%.o, $^) $(LDFLAGS)

-include $(DEP_FILES)

#implicit-rule macro for .o files
define OBJ_RULE
$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.$(1) ; $$(CXX) $$(CPPFLAGS) $$(CXXFLAGS) -c -o $$@ $$<
endef

#implicit-rule macro for .d files
define DEP_RULE
$(OBJ_DIR)/%.d: $(SOURCE_DIR)/%.$(1) ; $$(CXX) $$(CPPFLAGS) $$< -MM -MT $$@ -MT $$(subst .d,.o,$$@) -MF $$(subst .o,.d,$$@)
endef

#call the macros for each suffix
$(foreach suffix,$(SOURCE_SUFFIXES),$(eval $(call OBJ_RULE,$(suffix))))
$(foreach suffix,$(SOURCE_SUFFIXES),$(eval $(call DEP_RULE,$(suffix))))

#for reference, this is what the output of these macros would look like if hand-written, for the suffix cpp

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@  $<

$(OBJ_DIR)/%.d: $(SOURCE_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $< -MM -MT $@ -MT $(subst .d,.o,$@) -MF $(subst .o,.d,$@)

%.h :
	@echo
	@echo caught unsatisfied header : $@
	@echo
	
