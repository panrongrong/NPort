# Wind River Workbench generated Makefile.
# Do not edit!!!

# The file ".wrmakefile" is the template used by the Wind River Workbench to
# generate the makefiles of this project. Add user-specific build targets and
# make rules only(!) in this project's ".wrmakefile" file. These will then be
# automatically dumped into the makefiles.

WIND_HOME := $(subst \,/,$(WIND_HOME))
WIND_BASE := $(subst \,/,$(WIND_BASE))
WIND_USR := $(subst \,/,$(WIND_USR))
WRVX_COMPBASE := $(subst \,/,$(WRVX_COMPBASE))

all : pre_recursion subdirs_all post_recursion pre_build main_all post_build

TRACE=0
TRACEON=$(TRACE:0=@)
TRACE_FLAG=$(TRACEON:1=)

MAKEFILE := Makefile

BUILD_SPEC := default
SRC_DIR := .
PRJ_ROOT_DIR := D:/vxworkspace/FMSH/1234
WS_ROOT_DIR := D:/vxworkspace/FMSH

ALL_BUILD_SPECS := default default_rom default_romCompress default_romResident
ENABLED_BUILD_SPECS := $(ALL_BUILD_SPECS)

PROJECT_TYPE := VIP

BSP_DIR := D:/WindRiver69_new/WindRiver69_new/vxworks-6.9/target/config/fmsh_fmql_xj
BSP := fmsh_fmql_xj

VX_CPU_FAMILY := arm
CPU := _VX_ARMARCH7
TOOL_FAMILY := gnu
TOOL := gnu

ifeq ($(BUILD_SPEC),default)
OBJ_DIR := default
endif
ifeq ($(BUILD_SPEC),default_rom)
OBJ_DIR := default_rom
endif
ifeq ($(BUILD_SPEC),default_romCompress)
OBJ_DIR := default_romCompress
endif
ifeq ($(BUILD_SPEC),default_romResident)
OBJ_DIR := default_romResident
endif

ifeq ($(BUILD_SPEC),default)
PROJECT_TARGETS := target_vxWorks

SUB_TARGETS :=
endif
ifeq ($(BUILD_SPEC),default_rom)
PROJECT_TARGETS := target_vxWorks

SUB_TARGETS :=
endif
ifeq ($(BUILD_SPEC),default_romCompress)
PROJECT_TARGETS := target_vxWorks

SUB_TARGETS :=
endif
ifeq ($(BUILD_SPEC),default_romResident)
PROJECT_TARGETS := target_vxWorks

SUB_TARGETS :=
endif

VXPRJ_BUILDCMD := $(MAKE) -f $(PRJ_ROOT_DIR)/Makefile.mk $(MFLAGS)


WIND_SCOPETOOLS_BASE := $(subst \,/,$(WIND_SCOPETOOLS_BASE))

clean_scopetools :
	$(TRACE_FLAG)rm -rf $(PRJ_ROOT_DIR)/.coveragescope/db

CLEAN_STEP := clean_scopetools


-include *.makefile

main_all : check_objectdir external_build $(PROJECT_TARGETS)
	@echo "make: built targets of `pwd`"

check_objectdir :
	@if [ ! -d "$(OBJ_DIR)" ]; then\
		mkdir -p $(OBJ_DIR);\
	fi

# entry point for extending the build
external_build ::
	@echo ""

# main entry point for pre processing prior to the recursion
pre_recursion ::
	@echo ""

# main entry point for post processing after the recursion
post_recursion ::
	@echo ""

# main entry point for pre processing prior to the build
pre_build :: $(PRE_BUILD_STEP) generate_sources
	@echo ""

# entry point for generating sources prior to the build
generate_sources ::
	@echo ""

# main entry point for post processing after the build
post_build :: $(POST_BUILD_STEP) deploy_output
	@echo ""

# entry point for deploying output after the build
deploy_output ::
	@echo ""

clean :: subdirs_clean external_clean $(CLEAN_STEP)

# entry point for extending the build clean
external_clean ::
	@echo ""

target_% : force
	@echo "make: forwarding to make rule...";\
	"$(MAKE)" -f "$(MAKEFILE)" $(MFLAGS) BUILD_SPEC=$(BUILD_SPEC) $(OBJ_DIR)/$*

$(OBJ_DIR)/%.o : force
	@cd $(PRJ_ROOT_DIR);\
	"$(MAKE)" -f "$(MAKEFILE)" $(MFLAGS) BUILD_SPEC=$(BUILD_SPEC) compile_$*.o

force :

build_all_specs :
	@echo "building target default for ALL build-specs";\
	for spec in _dummy_ $(ALL_BUILD_SPECS); do\
		if [ "$$spec" = "_dummy_" ]; then\
			continue ;\
		fi;\
		echo " ";\
		echo "building all for build-spec '$$spec'";\
		"$(MAKE)" -f "$(MAKEFILE)" $(MFLAGS) BUILD_SPEC=$$spec DEBUG_MODE=$(DEBUG_MODE) TRACE=$(TRACE) || exit;\
	done

clean_all_specs :
	@echo "building target clean for ALL build-specs";\
	for spec in _dummy_ $(ALL_BUILD_SPECS); do\
		if [ "$$spec" = "_dummy_" ]; then\
			continue ;\
		fi;\
		echo " ";\
		echo "building clean for build-spec '$$spec'";\
		"$(MAKE)" -f "$(MAKEFILE)" $(MFLAGS) BUILD_SPEC=$$spec DEBUG_MODE=$(DEBUG_MODE) TRACE=$(TRACE) clean || exit;\
	done

build_enabled_specs :
	@echo "building target default for ENABLED build-specs";\
	for spec in _dummy_ $(ENABLED_BUILD_SPECS); do\
		if [ "$$spec" = "_dummy_" ]; then\
			continue ;\
		fi;\
		echo " ";\
		echo "building all for build-spec '$$spec'";\
		"$(MAKE)" -f "$(MAKEFILE)" $(MFLAGS) BUILD_SPEC=$$spec DEBUG_MODE=$(DEBUG_MODE) TRACE=$(TRACE) || exit;\
	done

clean_enabled_specs :
	@echo "building target clean for ENABLED build-specs";\
	for spec in _dummy_ $(ENABLED_BUILD_SPECS); do\
		if [ "$$spec" = "_dummy_" ]; then\
			continue ;\
		fi;\
		echo " ";\
		echo "building clean for build-spec '$$spec'";\
		"$(MAKE)" -f "$(MAKEFILE)" $(MFLAGS) BUILD_SPEC=$$spec DEBUG_MODE=$(DEBUG_MODE) TRACE=$(TRACE) clean || exit;\
	done

subdirs_all :

subdirs_clean :
