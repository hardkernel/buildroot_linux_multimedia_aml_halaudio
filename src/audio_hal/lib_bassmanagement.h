#ifndef LIB_BASSMANAGEMENT_H
#define LIB_BASSMANAGEMENT_H

int aml_bm_lowerpass_process(const void *buffer
                    , size_t bytes
                    , int sample_num
                    , int channel_num);
int aml_bass_management_init(int param_index);

#endif

