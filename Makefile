CXX ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -Werror
CORE = src/maze.cpp src/ga.cpp
EMCC ?= emcc
EMFLAGS = -std=c++11 -O2 -Wall -Wextra -Werror -sMODULARIZE -sEXPORT_ES6 -sALLOW_MEMORY_GROWTH \
  -sEXPORTED_FUNCTIONS=_mf_create,_mf_error,_mf_step,_mf_generation,_mf_best_fitness,_mf_reached_goal,_mf_width,_mf_height,_mf_shortest_path,_mf_genome_length,_mf_population_size,_mf_cells,_mf_paths,_mf_path_lengths,_malloc,_free \
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,HEAPU8,HEAP16,HEAP32

.PHONY: all test wasm determinism clean

all: build/mazefinder

build/mazefinder: $(CORE) src/main.cpp src/*.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -pthread $(CORE) src/main.cpp -o $@

build/tests: $(CORE) tests/test_main.cpp src/*.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(CORE) tests/test_main.cpp -o $@

test: build/tests
	./build/tests

# The browser build, committed so the site can copy it without a build step.
wasm: web/mazefinder.js

web/mazefinder.js: $(CORE) src/wasm_api.cpp src/*.h
	$(EMCC) $(EMFLAGS) -sENVIRONMENT=web $(CORE) src/wasm_api.cpp -o $@

# The same code built for Node, for the native-vs-WASM determinism test.
build/mazefinder-node.mjs: $(CORE) src/wasm_api.cpp src/*.h
	@mkdir -p build
	$(EMCC) $(EMFLAGS) -sENVIRONMENT=node $(CORE) src/wasm_api.cpp -o $@

determinism: build/mazefinder build/mazefinder-node.mjs
	node tests/determinism.mjs

clean:
	rm -rf build
