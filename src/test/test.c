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

int hdmi_source (audio_hw_device_t *dev) {
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
	sources.ext.device.type = AUDIO_DEVICE_IN_HDMI;

	memset(&sinks, 0 , sizeof(struct audio_port_config));
	sinks.id = 2;
	sinks.role = AUDIO_PORT_ROLE_SINK;
	sinks.type = AUDIO_PORT_TYPE_DEVICE;
	sinks.ext.device.type = AUDIO_DEVICE_OUT_SPEAKER;

    rc = dev->create_audio_patch(dev,num_sources,&sources,num_sinks,&sinks,&halPatch);
	printf("audio patch handle=0x%x\n",halPatch);
    if(rc) {
		printf("create audio patch failed =%d\n",rc);
        return -1;
	}
    /* Android HDMI case, it sets the source gain, so we follow it*/
	sources.gain.values[0] = 100; 
	sources.config_mask = AUDIO_PORT_CONFIG_GAIN;
    dev->set_audio_port_config(dev,&sources);

    while(cnt <= 20) {
	    sleep(1);
		cnt++;
		printf("test HDMI running =%d\n",cnt);
		//sources.gain.values[0] = 20 - cnt;
		//sources.config_mask = AUDIO_PORT_CONFIG_GAIN;
		//dev->set_audio_port_config(dev,&sources);			
    }
    printf("release patch\n");
	rc = dev->release_audio_patch(dev,halPatch);
    if(rc) {
		printf("release audio patch failed =%d\n",rc);
        return -1;
	}


    return 0;
}

int linein_source (audio_hw_device_t *dev) {
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

    rc = dev->create_audio_patch(dev,num_sources,&sources,num_sinks,&sinks,&halPatch);
	printf("audio patch handle=0x%x\n",halPatch);
    if(rc) {
		printf("create audio patch failed =%d\n",rc);
        return -1;
	}
    /* Android HDMI case, it sets the source gain, so we follow it*/
	sources.gain.values[0] = 20;
	sources.config_mask = AUDIO_PORT_CONFIG_GAIN;
    dev->set_audio_port_config(dev,&sources);

    while(cnt <= 20) {
	    sleep(1);
		cnt++;
		printf("test linein running =%d\n",cnt);
		sources.gain.values[0] = 20 - cnt;
		sources.config_mask = AUDIO_PORT_CONFIG_GAIN;
		dev->set_audio_port_config(dev,&sources);		
    }
    printf("release patch\n");
	rc = dev->release_audio_patch(dev,halPatch);
    if(rc) {
		printf("release audio patch failed =%d\n",rc);
        return -1;
	}


    return 0;
}



int media_source( audio_hw_device_t *dev) {
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


	config.channel_mask = AUDIO_CHANNEL_OUT_STEREO;
	config.sample_rate  = 48000;
    config.format       = AUDIO_FORMAT_AC3;
	
    /* open the output stream */
	rc = dev->open_output_stream(dev, 
								 handle, 
								 AUDIO_DEVICE_OUT_SPEAKER,
								 AUDIO_OUTPUT_FLAG_DIRECT,
								 &config,
								 &stream_out,
								 NULL);
	if(rc) {
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

    rc = dev->create_audio_patch(dev,num_sources,&sources,num_sinks,&sinks,&halPatch);
	printf("audio patch handle=0x%x\n",halPatch);
    if(rc) {
		printf("create audio patch failed =%d\n",rc);
        return -1;
	}

	sources.gain.values[0] = 100; 
	sources.config_mask = AUDIO_PORT_CONFIG_GAIN;
    dev->set_audio_port_config(dev,&sources);

	sinks.gain.values[0] = 100; 
	sinks.config_mask = AUDIO_PORT_CONFIG_GAIN;
    dev->set_audio_port_config(dev,&sinks);


	fp_input = fopen("/data/test.ac3", "r+");
	if(fp_input == NULL) {
        printf("open input file failed\n");
		
	} else {
		/* write data into outputstream*/
	    while(1) {
			read_size = fread(temp_buf,1,512,fp_input);
			if(read_size <= 0) {
				printf("read error\n");
				fclose(fp_input);
				break;
			}
			
            //printf("read data=%d\n",read_size);
			stream_out->write(stream_out,temp_buf,read_size);
			//printf("after write data\n");

		}
	}

	stream_out->common.standby(&stream_out->common);

	dev->close_output_stream(dev,stream_out);

	/* release the audio patch*/
	rc = dev->release_audio_patch(dev,halPatch);
    if(rc) {
		printf("release audio patch failed =%d\n",rc);
        return -1;
	}



    return 0;
}

int main ( ) {
	audio_hw_device_t *dev;
    hw_module_t *mod = NULL;
    int rc;
	int cnt = 0;


    printf("test HDMI in function\n");
	/* get the audio module*/
    rc = audio_hw_device_get_module(&mod);
	printf("audio_hw_device_get_module rc=%d mod addr=%p\n",rc,mod);
	if(rc) {
        printf("audio_hw_device_get_module failed\n");
		return -1;

	}
		
    /* open the devce*/
	rc = audio_hw_device_open(mod, &dev);
	printf("audio_hw_device_open rc=%d dev=%p\n",rc,dev);
	if(rc) {
        printf("audio_hw_device_open failed\n");
		return -1;
	}


	// linein_source(dev);
	media_source(dev);

	//hdmi_source(dev);

#if 0
    while(cnt <= 20) {
	    sleep(1);
		cnt++;
		printf("waiting exit =%d\n",cnt);
    }
#endif
    /* close the audio device*/
	rc = audio_hw_device_close(dev);
	printf("audio_hw_device_close rc=%d\n",rc);

	
    return 0;
}


