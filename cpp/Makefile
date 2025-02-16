#.PHONY: all images fonts launch-openocd clean distclean
.PHONY: all launch-openocd clean distclean

APP_NAME = shapopad
REPO_DIR = $(shell git rev-parse --show-toplevel)
#LIB_DIR = $(REPO_DIR)/lib
INC_DIR = include
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin/$(BOARD)

#BOARD := pico
#BOARD := pico2
BOARD := pico_w

WIFI_SSID := ""
WIFI_PASS := ""

BIN_NAME = $(APP_NAME).uf2
ELF_NAME = $(APP_NAME).elf
BIN = $(BIN_DIR)/$(BIN_NAME)
ELF = $(BIN_DIR)/$(ELF_NAME)

APP_NAMESPACE = shapoco::$(APP_NAME)

#BMP_DIR = bmp
#IMAGES_CPP = $(SRC_DIR)/images.cpp
#IMAGES_HPP = $(SRC_DIR)/images.hpp
#IMAGES_SRC_LIST = $(wildcard $(BMP_DIR)/*.*)
#IMAGES_CPP_GEN_CMD = ./gen_bmp_array.py

#FONT4_NAME = font4
#FONT5_NAME = font5
#FONT12_NAME = font12
#FONT16_NAME = font16

#FONT_LIB_DIR = $(LIB_DIR)/shapoco/graphics
#FONT_SRC_DIR = $(SRC_DIR)/fonts
#FONT_COMMON_HPP = $(FONT_LIB_DIR)/tinyfont.hpp
#FONT_BMP_DIR = $(BMP_DIR)/fonts
#FONT_CPP_GEN_CMD = $(FONT_LIB_DIR)/gen_font_array.py

#FONT_HPP_LIST = \
#	$(FONT_SRC_DIR)/$(FONT4_NAME).hpp \
#	$(FONT_SRC_DIR)/$(FONT5_NAME).hpp \
#	$(FONT_SRC_DIR)/$(FONT12_NAME).hpp \
#	$(FONT_SRC_DIR)/$(FONT16_NAME).hpp
#
#FONT_CPP_LIST = \
#	$(FONT_SRC_DIR)/$(FONT4_NAME).cpp \
#	$(FONT_SRC_DIR)/$(FONT5_NAME).cpp \
#	$(FONT_SRC_DIR)/$(FONT12_NAME).cpp \
#	$(FONT_SRC_DIR)/$(FONT16_NAME).cpp

DEPENDENCY_LIST=\
	$(wildcard $(INC_DIR)/*.*) \
	$(wildcard $(INC_DIR)/**/*.*) \
	$(wildcard $(SRC_DIR)/*.*) \
	$(wildcard $(SRC_DIR)/**/*.*)

all: $(BIN)
#fonts: $(FONT_HPP_LIST)
#images: $(IMAGES_HPP)

#$(BIN): $(DEPENDENCY_LIST) $(FONT_HPP_LIST) CMakeLists.txt

$(BIN): $(DEPENDENCY_LIST) CMakeLists.txt
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) \
		&& cmake -DPICO_BOARD=$(BOARD) -DCMAKE_BUILD_TYPE=Debug .. \
		&& make -j
	mkdir -p $(BIN_DIR)
	cp $(BUILD_DIR)/$(BIN_NAME) $(BIN)
	cp $(BUILD_DIR)/$(ELF_NAME) $(ELF)
	@echo "------------------------------"
	@grep "todo" -r $(SRC_DIR) $(INC_DIR) || true
	@echo "------------------------------"
	@echo -n "UF2 File: "
	@du -h $(BIN)
	@echo -n "ELF File: "
	@du -h $(ELF)

$(ELF): $(BIN)

#$(IMAGES_HPP): $(IMAGES_CPP)
#	@echo -n ""
#
#$(IMAGES_CPP): $(IMAGES_SRC_LIST) $(IMAGES_CPP_GEN_CMD)
#	rm -f $(IMAGES_CPP) $(IMAGES_HPP)
#	@echo "#pragma once" >> $(IMAGES_HPP)
#	@echo >> $(IMAGES_HPP)
#	@echo "#include <stdint.h>" >> $(IMAGES_HPP)
#	@echo "namespace $(APP_NAMESPACE) {" >> $(IMAGES_HPP)
#	@echo >> $(IMAGES_HPP)
#	@echo "#include <stdint.h>" >> $(IMAGES_CPP)
#	@echo "namespace $(APP_NAMESPACE) {" >> $(IMAGES_CPP)
#	$(IMAGES_CPP_GEN_CMD) --outcpp $(IMAGES_CPP) --outhpp $(IMAGES_HPP) --src $(BMP_DIR)/bit_icons.png --name bmp_bit_icons
#	$(IMAGES_CPP_GEN_CMD) --outcpp $(IMAGES_CPP) --outhpp $(IMAGES_HPP) --src $(BMP_DIR)/meter_frame.png --name bmp_meter_frame
#	$(IMAGES_CPP_GEN_CMD) --outcpp $(IMAGES_CPP) --outhpp $(IMAGES_HPP) --src $(BMP_DIR)/icon_beat.png --name bmp_icon_beat
#	@echo >> $(IMAGES_HPP)
#	@echo "}" >> $(IMAGES_HPP)
#	@echo >> $(IMAGES_CPP)
#	@echo "}" >> $(IMAGES_CPP)

#$(FONT_SRC_DIR)/%.hpp: $(FONT_SRC_DIR)/%.cpp
#	@echo -n ""
#
#$(FONT_SRC_DIR)/%.cpp: $(FONT_BMP_DIR)/%.png $(FONT_CPP_GEN_CMD)
#	$(FONT_CPP_GEN_CMD) \
#		--src $< \
#		--name $(patsubst $(FONT_BMP_DIR)/%.png,%,$<) \
#		--outdir $(FONT_SRC_DIR) \
#		--cpp_namespace $(APP_NAMESPACE)::fonts \
#		$(shell cat $(patsubst %.png,%.args.txt,$<))

ifneq ("$(wildcard debug.mk)", "")
include debug.mk
else ifneq ("$(shell which cmd.exe)", "")
include debug/wsl/debug.sample.mk
endif

debug-run: $(ELF)
	gdb-multiarch \
		-ex "target remote localhost:3333" \
		-x debug/launch.gdb \
		-q $(ELF)

launch-openocd:
	cd $(OPENOCD_REPO_PATH) && \
	sudo src/openocd \
		-f tcl/interface/cmsis-dap.cfg \
		-c 'adapter speed 10000' \
		-f tcl/target/rp2350.cfg \
		-s tcl

objclean:
	rm -f \
		$(BUILD_DIR)/*.uf2 \
		$(BUILD_DIR)/*.elf

clean: objclean
	rm -rf $(BUILD_DIR)

distclean: clean
	rm -rf $(BIN_DIR)
