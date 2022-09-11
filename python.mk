TMP ?= ./tmp

PYTHON_VENV=$(TMP)/venv
PYTHON=$(PYTHON_VENV)/bin/python3
PIP=$(PYTHON_VENV)/bin/pip3

$(PYTHON_VENV):
	python3 -m venv $(PYTHON_VENV)
	$(PIP) install --upgrade pip
	$(PIP) install $(PYTHON_DEPS)

$(PYTHON): $(PYTHON_VENV)

$(PIP3):   $(PYTHON_VENV)
