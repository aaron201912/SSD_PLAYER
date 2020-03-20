INC += $(wildcard $(foreach m,$(LIBS_PATH),$(m)/*))
LIBS += $(foreach m,$(DEP),-l$(m))