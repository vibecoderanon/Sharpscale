#---------------------------------------------------------------------------------
# Sharpscale-NX Top-Level Makefile
#---------------------------------------------------------------------------------
.PHONY: all plugin overlay clean

all: plugin overlay

plugin:
	@$(MAKE) -C plugin -f Makefile

overlay:
	@$(MAKE) -C overlay -f Makefile

clean:
	@$(MAKE) -C plugin -f Makefile clean
	@$(MAKE) -C overlay -f Makefile clean
