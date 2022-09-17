TMP ?= tmp

ARDUINO_CLI_VER=0.27.1
ARDUINO_CLI_PLAT=macOS_ARM64
ARDUINO_CLI_URL=https://github.com/arduino/arduino-cli/releases/download/$(ARDUINO_CLI_VER)/arduino-cli_$(ARDUINO_CLI_VER)_$(ARDUINO_CLI_PLAT).tar.gz
ARDUINO_CLI=$(TMP)/arduino-cli

$(ARDUINO_CLI):
	mkdir -p $(@D)
	curl -L $(ARDUINO_CLI_URL) | tar xzv -C $(@D)
	$@ version
