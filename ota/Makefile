DB_BUILD_TOP:=$(shell pwd)
DB_OUT_PATH:=$(DB_BUILD_TOP)/out
DB_ALKAID_PROJ:=/home/malloc.peng/p2_sdk/project/configs/current.configs
include $(DB_ALKAID_PROJ)
-include $(MODULE)/$(MODULE).mk
ifneq ($(USE_X86), 1)
include $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/toolchain.mk
else
CC := gcc
CXX := g++
AR := ar
STRIP := strip
GCCFLAGS := -Wall -g
endif

ifeq ($(TOOLCHAIN), glibc)
ifeq ($(TOOLCHAIN_VERSION), 8.2.1)
	MODULES += otaunpack
endif
endif

LIBS_PATH := ./internal/

INTERNAL_LIBS := $(notdir $(wildcard $(foreach m,$(LIBS_PATH),$(m)/*)))
MODULES_CLEAN := $(foreach m,$(MODULES),$(m)_clean)
MODULES_APP_ALL := $(foreach m,$(MODULES),$(m)_app_all)
MODULES_APP_CLEAN := $(foreach m,$(MODULES),$(m)_app_clean)
MODULES_OBJS_ALL := $(foreach m,$(INTERNAL_LIBS),$(m)_obj_all)
MODULES_OBJS_CLEAN := $(foreach m,$(INTERNAL_LIBS),$(m)_obj_clean)
MODULES_LIBS_ALL := $(foreach m,$(INTERNAL_LIBS),$(m)_lib_all)
MODULES_LIBS_CLEAN := $(foreach m,$(INTERNAL_LIBS),$(m)_lib_clean)
MODULES_INSTALL := $(foreach m,$(MODULES),$(m)_install)

export PROJ_ROOT CHIP PRODUCT BOARD TOOLCHAIN TOOLCHAIN_VERSION TOOLCHAIN_REL KERNEL_VERSION CUSTOMER_OPTIONS MOD_PREFIX INTERFACE_ENABLED INTERFACE_DISABLED MHAL_ENABLED MHAL_DISABLED
export DB_BUILD_TOP
export DB_OUT_PATH
export DB_PROJECT
export DB_ALKAID_PROJ
export TARGET_OUT
export LIBS_PATH

.PHONY: all lib obj clean $(MODULES) $(MODULES_CLEAN) $(MODULES_APP_ALL) $(MODULES_APP_CLEAN) $(MODULES_OBJS_ALL) $(MODULES_OBJS_CLEAN) $(MODULES_LIBS_ALL) $(MODULES_LIBS_CLEAN)

all:
	$(foreach mod,$(MODULES),$(MAKE) $(mod);)
clean:
	$(foreach mod,$(MODULES),$(MAKE) $(patsubst %,%_clean,$(mod));)
	@rm -rvf MakefileTmp_*
	@rm $(DB_OUT_PATH) -rf
	@rm ./obj/ -rf

install:
	@mkdir -p $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/
	@$(MAKE) $(MODULES_INSTALL)

module_all:
ifneq ($(DEP), )
	#@$(MAKE) $(foreach m,$(DEP),$(m)_obj_all)
	#@$(MAKE) $(foreach m,$(DEP),$(m)_lib_all)
endif

module_clean:
ifneq ($(DEP), )
	#@$(MAKE) $(foreach m,$(DEP),$(m)_obj_clean)
	#@$(MAKE) $(foreach m,$(DEP),$(m)_lib_clean)
endif

module_install:
	@$(STRIP) --strip-unneeded $(DB_OUT_PATH)/app/$(MODULE)
	@mkdir -p $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/$(MODULE)/
	@cp -vrf $(DB_OUT_PATH)/app/$(MODULE) $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/$(MODULE)/
ifneq ($(MODULE_REL_FILES), )
	$(foreach n,$(MODULE_REL_FILES),cp -rfv $(n) $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/$(MODULE)/;)
endif
$(MODULES_INSTALL):
	@$(MAKE) MODULE=$(patsubst %_install,%,$@) module_install

$(MODULES):
	@$(MAKE) MODULE=$@ module_all
	@$(MAKE) $(patsubst %,%_app_all,$@)

$(MODULES_CLEAN):
	@$(MAKE) $(patsubst %_clean,%_app_clean,$@)
	@$(MAKE) MODULE=$(patsubst %_clean,%,$@) module_clean

$(MODULES_APP_ALL):
	@$(MAKE) MODULE=$(patsubst %_app_all,%,$@) -f ./MakeApp.mk all

$(MODULES_APP_CLEAN):
	@$(MAKE) MODULE=$(patsubst %_app_clean,%,$@) -f ./MakeApp.mk clean

$(MODULES_OBJS_ALL):
	@$(MAKE) MODULE_OBJ=$(patsubst %_obj_all,%,$@) MODULE_OBJ_SRC=$(filter %$(patsubst %_obj_all,%,$@),$(wildcard $(foreach m,$(LIBS_PATH),$(m)/*))) -f ./MakeObj.mk all

$(MODULES_OBJS_CLEAN):
	@$(MAKE) MODULE_OBJ=$(patsubst %_obj_clean,%,$@) MODULE_OBJ_SRC=$(filter %$(patsubst %_obj_clean,%,$@),$(wildcard $(foreach m,$(LIBS_PATH),$(m)/*))) -f ./MakeObj.mk clean

$(MODULES_LIBS_ALL):
	@$(MAKE) MODULE_LIB=$(patsubst %_lib_all,%,$@) LIB_TYPE=static -f ./MakeLib.mk all
	@$(MAKE) MODULE_LIB=$(patsubst %_lib_all,%,$@) LIB_TYPE=dynamic -f ./MakeLib.mk all

$(MODULES_LIBS_CLEAN):
	@$(MAKE) MODULE_LIB=$(patsubst %_lib_clean,%,$@) LIB_TYPE=static -f ./MakeLib.mk clean
	@$(MAKE) MODULE_LIB=$(patsubst %_lib_clean,%,$@) LIB_TYPE=dynamic -f ./MakeLib.mk clean