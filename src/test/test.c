#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>



#include "audio_hal.h"



int format_changed_callback(audio_callback_info_t *info, void * args)
{
    printf("%s format=0x%x\n", __func__, *(audio_format_t*)(args));

    return 0;
}



int spdif_source(audio_hw_device_t *dev)
{
    int rc;

    unsigned int num_sources = 0;
    unsigned int num_sinks = 0;
    struct audio_port_config sources;
    struct audio_port_config sinks;
    audio_patch_handle_t halPatch = AUDIO_PATCH_HANDLE_NONE;
    int cnt = 0;



    dev->set_parameters(dev, "audio_latency=20");


    /* create the audio patch*/
    memset(&sources, 0 , sizeof(struct audio_port_config));
    num_sources = 1;
    num_sinks = 1;
    sources.id = 1;
    sources.role = AUDIO_PORT_ROLE_SOURCE;
    sources.type = AUDIO_PORT_TYPE_DEVICE;
    sources.ext.device.type = AUDIO_DEVICE_IN_SPDIF;

    memset(&sinks, 0 , sizeof(struct audio_port_config));
    sinks.id = 2;
    sinks.role = AUDIO_PORT_ROLE_SINK;
    sinks.type = AUDIO_PORT_TYPE_DEVICE;
    sinks.ext.device.type = AUDIO_DEVICE_OUT_SPEAKER;

    rc = dev->create_audio_patch(dev, num_sources, &sources, num_sinks, &sinks, &halPatch);
    printf("audio patch handle=0x%x\n", halPatch);
    if (rc) {
        printf("create audio patch failed =%d\n", rc);
        return -1;
    }
    /* Android HDMI case, it sets the source gain, so we follow it*/
    sources.gain.values[0] = 100;
    sources.config_mask = AUDIO_PORT_CONFIG_GAIN;
    dev->set_audio_port_config(dev, &sources);


    audio_callback_info_t  callback_info;
    callback_info.id = getpid();
    callback_info.type = AML_AUDIO_CALLBACK_FORMATCHANGED;
    audio_callback_func_t  callback_func = format_changed_callback;

    dev->install_callback_audio_patch(dev, halPatch, &callback_info, callback_func);


    while (cnt <= 20) {
        sleep(1);
        cnt++;
        printf("test HDMI running =%d\n", cnt);
        //sources.gain.values[0] = 20 - cnt;
        //sources.config_mask = AUDIO_PORT_CONFIG_GAIN;
        //dev->set_audio_port_config(dev,&sources);
    }
    dev->remove_callback_audio_patch(dev, halPatch, &callback_info);

    printf("release patch func=%p\n", dev->release_audio_patch);
    rc = dev->release_audio_patch(dev, halPatch);
    if (rc) {
        printf("release audio patch failed =%d\n", rc);
        return -1;
    }


    return 0;
}

int linein_source(audio_hw_device_t *dev)
{
    int rc;

    unsigned int num_sources = 0;
    unsigned int num_sinks = 0;
    struct audio_port_config sources;
    struct audio_port_config sinks;
    audio_patch_handle_t halPatch = AUDIO_PATCH_HANDLE_NONE;
    int cnt = 0;



    /* create the audio patch*/
    memset(&sources, 0 , sizeof(struct audio_port_config));
    num_sources = 1;
    num_sinks = 1;
    sources.id = 1;
    sources.role = AUDIO_PORT_ROLE_SOURCE;
    sources.type = AUDIO_PORT_TYPE_DEVICE;
    sources.ext.device.type = AUDIO_DEVICE_IN_LINE;

    memset(&sinks, 0 , sizeof(struct audio_port_config));
    sinks.id = 2;
    sinks.role = AUDIO_PORT_ROLE_SINK;
    sinks.type = AUDIO_PORT_TYPE_DEVICE;
    sinks.ext.device.type = AUDIO_DEVICE_OUT_SPEAKER;

    rc = dev->create_audio_patch(dev, num_sources, &sources, num_sinks, &sinks, &halPatch);
    printf("audio patch handle=0x%x\n", halPatch);
    if (rc) {
        printf("create audio patch failed =%d\n", rc);
        return -1;
    }
    /* Android HDMI case, it sets the source gain, so we follow it*/
    sources.gain.values[0] = 100;
    sources.config_mask = AUDIO_PORT_CONFIG_GAIN;
    dev->set_audio_port_config(dev, &sources);

    while (cnt <= 20) {
        sleep(1);
        cnt++;
        printf("test linein running =%d\n", cnt);
    }
    printf("release patch\n");
    rc = dev->release_audio_patch(dev, halPatch);
    if (rc) {
        printf("release audio patch failed =%d\n", rc);
        return -1;
    }


    return 0;
}



int media_source(audio_hw_device_t *dev, audio_format_t  format)
{
    int rc = 0;
    audio_io_handle_t handle;
    audio_stream_out_t *stream_out = NULL;
    struct audio_config config;
    unsigned int num_sources = 0;
    unsigned int num_sinks = 0;
    struct audio_port_config sources;
    struct audio_port_config sinks;
    audio_patch_handle_t halPatch = AUDIO_PATCH_HANDLE_NONE;
    FILE *fp_input = NULL;
    char *temp_buf[1024];
    int read_size = 0;
    char * file_name = NULL;

    if (format == AUDIO_FORMAT_AC3) {
        file_name = "/data/test.ac3";
    } else if (format == AUDIO_FORMAT_DTS) {
        file_name = "/data/test.dts";
    } else {
        return 0;
    }

    config.channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    config.sample_rate  = 48000;
    config.format       = format;

    /* open the output stream */
    rc = dev->open_output_stream(dev,
                                 handle,
                                 AUDIO_DEVICE_OUT_SPEAKER,
                                 AUDIO_OUTPUT_FLAG_DIRECT,
                                 &config,
                                 &stream_out,
                                 NULL);
    if (rc) {
        printf("open output stream failed\n");
        return -1;

    }

    /* create the media audio patch */
    memset(&sources, 0 , sizeof(struct audio_port_config));
    num_sources = 1;
    num_sinks = 1;
    sources.id = 1;
    sources.role = AUDIO_PORT_ROLE_SOURCE;
    sources.type = AUDIO_PORT_TYPE_MIX;


    memset(&sinks, 0 , sizeof(struct audio_port_config));
    sinks.id = 2;
    sinks.role = AUDIO_PORT_ROLE_SINK;
    sinks.type = AUDIO_PORT_TYPE_DEVICE;
    sinks.ext.device.type = AUDIO_DEVICE_OUT_SPEAKER;

    rc = dev->create_audio_patch(dev, num_sources, &sources, num_sinks, &sinks, &halPatch);
    printf("audio patch handle=0x%x\n", halPatch);
    if (rc) {
        printf("create audio patch failed =%d\n", rc);
        return -1;
    }

    sources.gain.values[0] = 100;
    sources.config_mask = AUDIO_PORT_CONFIG_GAIN;
    dev->set_audio_port_config(dev, &sources);

    sinks.gain.values[0] = 100;
    sinks.config_mask = AUDIO_PORT_CONFIG_GAIN;
    dev->set_audio_port_config(dev, &sinks);


    fp_input = fopen(file_name, "r+");
    if (fp_input == NULL) {
        printf("open input file failed\n");

    } else {
        /* write data into outputstream*/
        while (1) {
            read_size = fread(temp_buf, 1, 512, fp_input);
            if (read_size <= 0) {
                printf("read error\n");
                fclose(fp_input);
                break;
            }

            //printf("read data=%d\n",read_size);
            stream_out->write(stream_out, temp_buf, read_size);
            //printf("after write data\n");

        }
    }

    stream_out->common.standby(&stream_out->common);

    dev->close_output_stream(dev, stream_out);

    /* release the audio patch*/
    rc = dev->release_audio_patch(dev, halPatch);
    if (rc) {
        printf("release audio patch failed =%d\n", rc);
        return -1;
    }



    return 0;
}

int main(int argc, char *argv[])
{
    audio_hw_device_t *dev;
    hw_module_t *mod = NULL;
    int rc;
    int cnt = 0;


    if (argc != 2 && argc != 3) {
        printf("cmd should be: \n");
        printf("************************\n");
        printf("test spdinfin \n");
        printf("test linein \n");
        printf("test mediain ac3 or dts\n");
        printf("************************\n");


        return -1;
    }
    /* get the audio module*/
    rc = audio_hw_device_get_module(&mod);
    printf("audio_hw_device_get_module rc=%d mod addr=%p\n", rc, mod);
    if (rc) {
        printf("audio_hw_device_get_module failed\n");
        return -1;

    }

    /* open the devce*/
    rc = audio_hw_device_open(mod, &dev);
    printf("audio_hw_device_open rc=%d dev=%p\n", rc, dev);
    if (rc) {
        printf("audio_hw_device_open failed\n");
        return -1;
    }

    if (strcmp(argv[1], "spdifin") == 0) {
        spdif_source(dev);
    } else if (strcmp(argv[1], "linein") == 0) {
        linein_source(dev);
    } else if (strcmp(argv[1], "mediain") == 0) {
        audio_format_t  format = AUDIO_FORMAT_INVALID;
        if (strcmp(argv[2], "ac3") == 0) {
            format = AUDIO_FORMAT_AC3;
        } else if (strcmp(argv[2], "dts") == 0) {
            format = AUDIO_FORMAT_DTS;
        }
        media_source(dev, format);
    }else if (strcmp(argv[1], "all") == 0){
        //run all the case
        
        spdif_source(dev);
        linein_source(dev);
        media_source(dev, AUDIO_FORMAT_AC3);
        media_source(dev, AUDIO_FORMAT_DTS);
        

    }

#if 0
    while (cnt <= 20) {
        sleep(1);
        cnt++;
        printf("waiting exit =%d\n", cnt);
    }
#endif
    /* close the audio device*/
    rc = audio_hw_device_close(dev);
    printf("audio_hw_device_close rc=%d\n", rc);


    return 0;
}


