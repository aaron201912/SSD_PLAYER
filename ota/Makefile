DB_BUILD_TOP:=$(shell pwd)
DB_OUT_PATH:=$(DB_BUILD_TOP)/out
DB_ALKAID_PROJ:=/home/malloc.peng/203_sdk/project/configs/current.configs
DB_3PARTY_PATH:=$(DB_BUILD_TOP)/3rdparty
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

MODULES += otaunpack

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
3RD_PARTY := $(foreach m,$(notdir $(wildcard $(DB_3PARTY_PATH)/*)),$(m)_3rd_party_all)

export PROJ_ROOT CHIP PRODUCT BOARD TOOLCHAIN TOOLCHAIN_VERSION TOOLCHAIN_REL KERNEL_VERSION CUSTOMER_OPTIONS MOD_PREFIX INTERFACE_ENABLED INTERFACE_DISABLED MHAL_ENABLED MHAL_DISABLED
export DB_BUILD_TOP
export DB_OUT_PATH
export DB_PROJECT
export DB_ALKAID_PROJ
export TARGET_OUT
export LIBS_PATH
export DB_3PARTY_PATH

.PHONY: all 3rdparty lib obj clean install depend_internal depend_internal_clean depend_3rd_party $(MODULES) $(MODULES_CLEAN) $(MODULES_APP_ALL) $(MODULES_APP_CLEAN) $(MODULES_OBJS_ALL) $(MODULES_OBJS_CLEAN) $(MODULES_LIBS_ALL) $(MODULES_LIBS_CLEAN)

-include $(foreach mod,$(MODULES_IN),$(mod)/dep.mk)

all:
	@$(MAKE) MODULES_IN="$(MODULES)" depend_internal
ifneq ($(MODULES_APP_ALL), )
	@$(MAKE) $(MODULES_APP_ALL)
endif

3rdparty:
	@$(MAKE) MODULES_IN="$(MODULES)" depend_3rd_party

clean:
	@$(MAKE) $(foreach mod,$(MODULES),$(patsubst %,%_clean,$(mod)))
	@$(MAKE) MODULES_IN="$(MODULES)" depend_internal_clean
	@rm $(DB_OUT_PATH) -rf
	@rm ./obj/ -rf

install:
	@mkdir -p $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/
	@$(MAKE) $(MODULES_INSTALL)

depend_internal:
ifneq ($(DEP), )
	@$(MAKE) $(foreach m,$(sort $(DEP)),$(m)_obj_all)
	@$(MAKE) $(foreach m,$(sort $(DEP)),$(m)_lib_all)
endif

depend_internal_clean:
ifneq ($(DEP), )
	@$(MAKE) $(foreach m,$(sort $(DEP)),$(m)_obj_clean)
	#@$(MAKE) $(foreach m,$(sort $(DEP)),$(m)_lib_clean)
endif

depend_3rd_party:
ifneq ($(3RD_PARTY_DEP), )
	@$(MAKE) $(foreach m,$(sort $(3RD_PARTY_DEP)),$(m)_3rd_party_all)
endif

module_install:
	@$(STRIP) --strip-unneeded $(DB_OUT_PATH)/app/$(MODULE)
	@mkdir -p $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/$(MODULE)/
	@cp -vrf $(DB_OUT_PATH)/app/$(MODULE) $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/$(MODULE)/
ifneq ($(MODULE_REL_FILES), )
	$(foreach n,$(MODULE_REL_FILES),cp -rfv $(n) $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/$(MODULE)/;)
endif

$(3RD_PARTY):
	@$(MAKE) -C $(DB_3PARTY_PATH)/$(patsubst %_3rd_party_all,%,$@)

$(MODULES_INSTALL):
	@$(MAKE) MODULE=$(patsubst %_install,%,$@) module_install

$(MODULES):
	@$(MAKE) MODULES_IN=$@ depend_3rd_party
	@$(MAKE) MODULES_IN=$@ depend_internal
	@$(MAKE) $(patsubst %,%_app_all,$@)

$(MODULES_CLEAN):
	@$(MAKE) $(patsubst %_clean,%_app_clean,$@)
	@$(MAKE) MODULES_IN=$@ depend_internal_clean

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
