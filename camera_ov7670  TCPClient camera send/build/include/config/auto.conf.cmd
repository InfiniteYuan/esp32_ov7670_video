deps_config := \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/app_trace/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/aws_iot/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/bt/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/esp32/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/components/network/espmqtt/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/ethernet/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/fatfs/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/freertos/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/heap/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/libsodium/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/log/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/lwip/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/mbedtls/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/openssl/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/components/platforms/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/pthread/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/spi_flash/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/spiffs/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/tcpip_adapter/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/wear_levelling/Kconfig \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/examples/camera_ov7670/components/camera/Kconfig.projbuild \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/examples/camera_ov7670/main/Kconfig.projbuild \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/yuanmingfu/ESP32WORK/esp-iot-solution/esp-iot-solution/submodule/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
