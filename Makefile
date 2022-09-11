TMP=./tmp
MINIFY=$(TMP)/minify-html
PYTHON_DEPS=flask-sock

include arduino-cli.mk
include python.mk

$(MINIFY):
	curl https://wilsonl.in/minify-html/bin/0.10.0-macos-x86_64 -o $(@)
	chmod +x $@

$(TMP)/index.min.html: $(MINIFY) html/index.html
	$(MINIFY) --output $(@) --keep-closing-tags --minify-js --minify-css html/index.html

.PHONY: flow
flow: $(ARDUINO_CLI) $(PYTHON)

.PHONY: sim
sim: export WORDCLOCK_INDEX_HTML = $(TMP)/index.min.html
sim: $(PYTHON) $(PIP) $(TMP)/index.min.html
	$(PYTHON_VENV)/bin/flask --app $(@)/app.py run

.PHONY: clean
clean:
	rm -rf $(TMP)
