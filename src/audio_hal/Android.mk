    LOCAL_PATH := $(call my-dir)
	
    include $(CLEAR_VARS)

    LOCAL_MODULE := libhalaudio

    LOCAL_SRC_FILES := audio_hw.c \
	audio_hw_utils.c \
	audio_hwsync.c \
	audio_hw_profile.c \
	aml_hw_mixer.c \
	alsa_manager.c \
	aml_audio_stream.c \
	alsa_config_parameters.c \
	aml_ac3_parser.c \
	aml_dcv_dec_api.c \
	aml_dca_dec_api.c \
	alsa_device_parser.c \
	audio_post_process.c \
	aml_avsync_tuning.c \
	audio_format_parse.c \
	dolby_lib_api.c \
	audio_route.c \
	spdif_encoder_api.c \
	aml_output_manager.c \
	aml_input_manager.c \
	aml_callback_api.c \
	aml_dec_api.c \
	aml_datmos_api.c \
	aml_sample_conv.c \
	aml_mat_parser.c \
	aml_spdif_in.c \
	aml_channel_map.c \
	aml_audio_volume.c \
	aml_bm_api.c \
	aml_audio_level.c \
	aml_audio_ease.c \
	standard_alsa_manager.c \
	aml_config_parser.c \
	aml_audio_delay.c \
	audio_eq_drc_compensation.cpp \
	audio_eq_drc_parser.cpp \
	aml_datmos_config.cpp

    LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/../include   \
        $(LOCAL_PATH)/../tinyalsa/include \
        $(LOCAL_PATH)/../utils/include \
        $(LOCAL_PATH)/../utils/ini/include \
		$(LOCAL_PATH)/../libms12/include \
		external/alsa-lib/include \

    LOCAL_SHARED_LIBRARIES := \
        liblog  libtinyalsa \
		libamaudioutils libcjson libasound

LOCAL_REQUIRED_MODULES += aml_audio_config.json
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -DDATMOS -DMAX_OUTPUT_CH=8 \
		-DBYTES_PER_SAMPLE=4 -DUSE_ALSA_PLUGINS \
		-Wall

ifeq ($(TARGET_DEVICE),$(filter $(TARGET_DEVICE), s400_sbr))
       LOCAL_CFLAGS += -DUSE_AUDIOSERVICE_S400_SBR
endif

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include

include $(BUILD_SHARED_LIBRARY)


# Install script and config files
include $(CLEAR_VARS)
LOCAL_MODULE := aml_audio_config.json
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := ../../config/8ch_aml_audio_config.json
include $(BUILD_PREBUILT)

