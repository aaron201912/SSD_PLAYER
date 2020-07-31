INC += $(DB_BUILD_TOP)/internal/otaunpack
LIBS += -lotaunpack
USE_FB = 1
ifeq ($(USE_FB), 1)
DEP += ui
3RD_PARTY_DEP += png jpeg z
endif
