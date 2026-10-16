SKM ?= skm
SKM_CXX ?= clang++
CXX_STD ?= c++17
CPPFLAGS ?=
CXXFLAGS ?=
LDFLAGS ?=

RUNTIME_SOURCES := gui.cpp utilities.cpp observation_analysis.cpp observation_plan.cpp
HEADERS := $(wildcard *.h)

.PHONY: all run test clean

all: build/star-map

build:
	mkdir -p build

build/star-map: star_map.cpp $(RUNTIME_SOURCES) $(HEADERS) | build
	$(SKM) $(SKM_CXX) -std=$(CXX_STD) -I. $(CPPFLAGS) $(CXXFLAGS) star_map.cpp $(RUNTIME_SOURCES) $(LDFLAGS) -o $@

build/gui-regression: tests/gui_regression.cpp $(RUNTIME_SOURCES) $(HEADERS) | build
	$(SKM) $(SKM_CXX) -std=$(CXX_STD) -I. $(CPPFLAGS) $(CXXFLAGS) tests/gui_regression.cpp $(RUNTIME_SOURCES) $(LDFLAGS) -o $@

build/analysis-regression: tests/analysis_regression.cpp observation_analysis.cpp utilities.cpp $(HEADERS) | build
	$(SKM) $(SKM_CXX) -std=$(CXX_STD) -I. $(CPPFLAGS) $(CXXFLAGS) tests/analysis_regression.cpp observation_analysis.cpp utilities.cpp $(LDFLAGS) -o $@

build/plan-regression: tests/plan_regression.cpp observation_plan.cpp observation_analysis.cpp utilities.cpp $(HEADERS) | build
	$(SKM) $(SKM_CXX) -std=$(CXX_STD) -I. $(CPPFLAGS) $(CXXFLAGS) tests/plan_regression.cpp observation_plan.cpp observation_analysis.cpp utilities.cpp $(LDFLAGS) -o $@

run: build/star-map
	./build/star-map

test: build/gui-regression build/analysis-regression build/plan-regression
	./build/gui-regression
	./build/analysis-regression
	./build/plan-regression

clean:
	$(RM) -r build
