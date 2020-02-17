#ifndef _ATOOLS_H
#define _ATOOLS_H
#include <alsa/asoundlib.h>
#include <semaphore.h>
#define ALSA_PCM_NEW_HW_PARAMS_API

// WAVE file header format
typedef struct WAVE_HEADER_T {
    char riff[4];                      // RIFF string
    unsigned int overall_size;         // overall size of file in bytes
    char wave[4];                      // WAVE string
    char fmt_chunk_marker[4];          // fmt string with trailing null char
    unsigned int length_of_fmt;        // length of the format data
    unsigned int format_type;          // format type. see wave_format_t
    unsigned int channels;             // number of channels
    unsigned int sample_rate;          // sampling rate (blocks per second)
    unsigned int byterate;             // SampleRate * NumChannels * BitsPerSample/8
    unsigned int block_align;          // NumChannels * BitsPerSample/8
    unsigned int bits_per_sample;      // bits per sample, 8- 8bits, 16- 16 bits etc
    char data_chunk_header [4];        // data string or list string
    unsigned int data_size;            // data size
} wave_header_t;

typedef enum AUDIO_TYPE_T {
    WAVE, MP3, FLAC, ALAC, ACC,
} audio_type_t;

typedef enum WAVE_FORMAT_T {
    UNKNOWN        = 0,
    PCM,
    MS_ADPCM,
    A_LAW          = 0x6,
    MU_LAW,
    IMA_ADPCM      = 0x11,
    ITU_G723_ADPCM = 0x16,
    ITU_G721_ADPCM = 0x31,
    MPEG           = 0x50,
    EXPERIMENTAL   = 0xffff

} wave_format_t;

typedef struct AUDIO_HEADER_T {
    union {
        wave_header_t wave_header;
    } h;
    audio_type_t type;
    long num_samples;
    size_t sample_size;
    double duration;
    unsigned int channels;
    unsigned int data_rate;
    unsigned int sample_rate;
    unsigned int bits_per_sample;
    size_t data_size;
} audio_header_t;

#ifdef LITTLE_ENDIAN
#define BUF2NUM2(buf) ((buf)[0] | ((buf)[1] << 8))
#define BUF2NUM4(buf) ((BUF2NUM2((buf))) | (BUF2NUM2((buf)+2) << 16))
#else
#define BUF2NUM2(buf) ((buf)[1] | ((buf)[0] << 8))
#define BUF2NUM4(buf) ((BUF2NUM2((buf+2))) | (BUF2NUM2((buf)) << 16))
#endif

void WEAtoolsDestruct();
void WEAtoolsPlay();
void WEAtoolsInit(char *filename);

extern sem_t sem_bgm_start;
#endif

