# Ordinary contained preparation uses a writable copy of declared shared inputs.
ifneq ($(strip $(PREPARATION_ROOT)),)
ifneq ($(PREPARATION_CHILD),1)
PREPARATION_DISPATCH := 1
endif
endif
ifeq ($(PREPARATION_DISPATCH),1)
include make/preparation.mk
else
include make/application.mk
endif
