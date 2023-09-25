# project config
BUILD_DIR := build
EXECUTABLE := asvii
FRAMES_FOLDER := ./frames/
CORES := $(shell sysctl -n hw.logicalcpu)

# compiler config
COMPILER := clang
FLAGS := -c -g -Wall -Werror -pedantic-errors

# source files
RENDER_FILES := render.c
VIDEO_FILES := video.c bitmap.c
FILES := main.c $(RENDER_FILES) $(VIDEO_FILES)
OBJECTS := $(addprefix $(BUILD_DIR)/,$(patsubst %.c,%.o,$(FILES)))

.PHONY: $(EXECUTABLE)
$(EXECUTABLE): $(BUILD_DIR)/$(EXECUTABLE)

.PRECIOUS: $(BUILD_DIR)/. $(BUILD_DIR)%/.

$(BUILD_DIR)/.:
	mkdir -p $@

$(BUILD_DIR)%/.:
	mkdir -p $@

.SECONDEXPANSION:

$(BUILD_DIR)/%.o: src/%.c | $$(@D)/.
	$(COMPILER) $(FLAGS) $< -o $@

$(BUILD_DIR)/$(EXECUTABLE): $(OBJECTS)
	$(COMPILER) $^ -o ./$(BUILD_DIR)/$(EXECUTABLE)

run:
	make clean
	make -j$(shell lsproc)
	./$(BUILD_DIR)/$(EXECUTABLE) ./input/bad_apple.mp4

new:
	make clean
	make -j$(shell lsproc)

clean:
	rm -rf build
	mkdir build
