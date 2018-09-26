PROJECT_NAME := arduhal

include $(IDF_PATH)/make/project.mk

gdb:
	$(Q) xtensa-esp32-elf-gdb -ex 'target remote localhost:3333' build/$(PROJECT_NAME).elf
