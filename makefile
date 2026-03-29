NAME := Fanboy
BUILD_DIR := build

.PHONY: all
all: release

.PHONY: build_dir
build_dir:
	@mkdir -p $(BUILD_DIR)

.PHONY: release
release: build_dir
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
	@echo "Release build created! Check in ${BUILD_DIR}"

.PHONY: debug
debug: build_dir
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug .. && cmake --build .
	@echo "Debug build created! Check in ${BUILD_DIR}"
