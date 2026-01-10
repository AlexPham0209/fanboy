NAME := Fanboy
BUILD_DIR := build

.PHONY: all
all: release

.PHONY: build_dir
build-dir:
	@mkdir -p $(BUILD_DIR)

.PHONY: release
release: build_dir
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Release -G"Unix Makefiles" .. && make
	@echo "Release build created! Check in ${BUILD_DIR}"

.PHONY: debug
debug: build_dir
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug -G"Unix Makefiles" .. && make
	@echo "Debug build created! Check in ${BUILD_DIR}"