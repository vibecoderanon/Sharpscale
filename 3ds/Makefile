#---------------------------------------------------------------------------------
# Sharpscale-3DS Top-Level Makefile
#---------------------------------------------------------------------------------
.PHONY: all plugin firm_patcher config_app clean

all: plugin firm_patcher config_app

plugin:
	@$(MAKE) -C plugin_3gx -f Makefile

firm_patcher:
	@$(MAKE) -C firm_patcher -f Makefile

config_app:
	@$(MAKE) -C config_app -f Makefile

clean:
	@$(MAKE) -C plugin_3gx -f Makefile clean
	@$(MAKE) -C firm_patcher -f Makefile clean
	@$(MAKE) -C config_app -f Makefile clean
