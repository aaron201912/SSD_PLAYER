LINK_TYPE ?= dynamic

INC  += $(PROJ_ROOT)/release/include
INC  += $(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/include/uapi/mstar
INC  += $(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/drivers/sstar/include

LIBS += -lrt -lpthread -lm -ldl
#
LIBS += -L$(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/mi_libs/$(LINK_TYPE)
LIBS += -L$(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/$(LINK_TYPE)
LIBS += -L./lib/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/$(LINK_TYPE)/

-include $(MODULE)/dep.mk
INC += $(wildcard $(foreach m,$(LIBS_PATH),$(m)/*))
LIBS += $(foreach m,$(DEP),-l$(m))

INC += $(foreach m,$(3RD_PARTY_DEP),$(DB_3PARTY_PATH)/$(m)/include)
LIBS += $(foreach m,$(3RD_PARTY_DEP),-l$(m))
LIBS += $(foreach m,$(3RD_PARTY_DEP),-L$(DB_3PARTY_PATH)/$(m)/lib/$(LINK_TYPE))


