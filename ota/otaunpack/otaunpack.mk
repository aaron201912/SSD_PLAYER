LINK_TYPE := static
INC += $(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/include/uapi/mstar/
INC += $(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/include/uapi/mtd/
INC += $(MOD_ROOT)/bmp/
INC += $(MOD_ROOT)/jpeg/
INC += $(MOD_ROOT)/png/
INC += $(MOD_ROOT)/raw/
INC += $(DB_BUILD_TOP)/3rdparty/include
SUBDIRS += $(MOD_ROOT)/bmp
SUBDIRS += $(MOD_ROOT)/jpeg
SUBDIRS += $(MOD_ROOT)/png
SUBDIRS += $(MOD_ROOT)/raw

LIBS += -L$(DB_BUILD_TOP)/3rdparty/lib/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/$(LINK_TYPE)
