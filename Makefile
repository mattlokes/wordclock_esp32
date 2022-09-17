TMP=./tmp
MINIFY=$(TMP)/minify-html
PYTHON_DEPS=flask-sock

SERIAL_PORT=/dev/cu.usbserial-0001
BAUD=115200

include arduino-cli.mk
include python.mk

$(MINIFY):
	mkdir -p $(@D)
	curl https://wilsonl.in/minify-html/bin/0.10.0-macos-x86_64 -o $(@)
	chmod +x $@

$(TMP)/index.min.html: $(MINIFY) html/index.html
	mkdir -p $(@D)
	# Minify HTML
	$(MINIFY) --output $(@).int --keep-closing-tags --minify-js --minify-css html/index.html
	# Convert all double quotes to single quotes
	cat $(@).int | sed "s/\"/'/g" > $@

$(TMP)/index.h: $(TMP)/index.min.html
	mkdir -p $(@D)
	echo "const String index_html = \"\c"  >  $@
	cat $^ | sed 's/\//\\\//g'       >> $@
	echo    "\";"                    >> $@


$(TMP)/AsyncTCP.zip: URL_ZIP=https://github.com/me-no-dev/AsyncTCP/archive/refs/heads/master.zip
$(TMP)/AsyncTCP.zip:
	curl -LJ $(URL_ZIP) -o $(@)

$(TMP)/ESPAsyncWebServer.zip: URL_ZIP=https://github.com/me-no-dev/ESPAsyncWebServer/archive/refs/heads/master.zip
$(TMP)/ESPAsyncWebServer.zip:
	curl -LJ $(URL_ZIP) -o $(@)

$(TMP)/Arduino_JSON.zip: URL_ZIP=https://github.com/arduino-libraries/Arduino_JSON/archive/refs/tags/0.1.0.zip
$(TMP)/Arduino_JSON.zip:
	curl -LJ $(URL_ZIP) -o $(@)


LIB_ZIPS=$(TMP)/AsyncTCP.zip \
         $(TMP)/ESPAsyncWebServer.zip \
         $(TMP)/Arduino_JSON.zip

SRC = src/wordclock.ino \
      $(TMP)/index.h

# Create tmp wordclock directory and copy all source to it
# so that arduino cli can compile it, because its so inflexible
$(TMP)/wordclock: $(SRC)
	mkdir -p $(@)
	cp -r $(SRC) $(@)

$(TMP)/bin/wordclock.ino.bin: export ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL=true
$(TMP)/bin/wordclock.ino.bin: $(ARDUINO_CLI) $(LIB_ZIPS) $(TMP)/wordclock
	$(ARDUINO_CLI) lib install --zip-path $(LIB_ZIPS)
	$(ARDUINO_CLI) compile -v \
                               -b esp32:esp32:esp32 \
                               --output-dir $(@D) \
                               $(TMP)/wordclock
.PHONY: compile
compile: $(TMP)/bin/wordclock.ino.bin

.PHONY: upload
upload: $(TMP)/bin/wordclock.ino.bin
	$(ARDUINO_CLI) upload -v \
                              -b esp32:esp32:esp32 \
	                      --input-dir $(<D) \
                              -p $(SERIAL_PORT)

.PHONY: monitor
monitor:
	$(ARDUINO_CLI) monitor \
                       -p $(SERIAL_PORT) \
                       --config baudrate=$(BAUD)

.PHONY: flow
flow: $(ARDUINO_CLI) $(PYTHON)

.PHONY: sim
sim: export WORDCLOCK_INDEX_HTML = $(TMP)/index.min.html
sim: $(PYTHON) $(PIP) $(TMP)/index.min.html
	$(PYTHON_VENV)/bin/flask --app $(@)/app.py run

.PHONY: clean
clean:
	rm -rf $(TMP)
