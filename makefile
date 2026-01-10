NAME := Fanboy
BUILD_DIR := build

.PHONY: all
all: release

.PHONY: build-dir
build-dir:
	@mkdir -p $(BUILD_DIR)

.PHONY: release
release: build_dir
	@cd $(BUILD_DIR) && cmake .. 
	cmake --build . --config Release

.PHONY: debug
debug: build_dir
	@cd $(BUILD_DIR) && cmake .. && 
	cmake --build . --config Debug