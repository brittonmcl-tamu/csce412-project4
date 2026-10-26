DOXYGEN ?= doxygen
CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++17
SRCS = main.cpp LoadBalancer.cpp WebServer.cpp
HEADERS = LoadBalancer.h WebServer.h Request.h
PACKAGE_FILES = $(SRCS) $(HEADERS) Makefile config.txt Doxyfile README.md tests/test_simulation.cpp tests/test_cli.py docs/index.html docs/style.css docs/design/index.html docs/design/styles.css docs/design/assets/statera-forge-logo.png docs/design/assets/workflow.svg docs/reports/20servers-100000cycles.log

.PHONY: all clean test docs package
all: LB

LB: $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

tests/test_simulation: tests/test_simulation.cpp LoadBalancer.cpp WebServer.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I. -o $@ tests/test_simulation.cpp LoadBalancer.cpp WebServer.cpp

test: LB tests/test_simulation
	./tests/test_simulation
	python3 tests/test_cli.py

docs:
	$(DOXYGEN) Doxyfile

package:
	python3 -c 'import sys,zipfile; archive=zipfile.ZipFile("project4-source.zip","w",zipfile.ZIP_DEFLATED); [archive.write(path) for path in sys.argv[1:]]; archive.close()' $(PACKAGE_FILES)

clean:
	rm -f LB *.o tests/test_simulation
