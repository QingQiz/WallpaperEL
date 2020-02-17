#include <string.h>
#include <signal.h>

#include "options.h"
#include "atools.h"
#include "debug.h"

snd_pcm_t *handler;
snd_pcm_hw_params_t *params;

sem_t sem_bgm_start;
extern int sig_exit;

char *org_data = NULL;
size_t org_size;

char *pcm_data = NULL;
size_t pcm_size;

audio_header_t header;


static char WEAtoolsReadDataFromPcm(void *p, size_t size) {
    static char *position = NULL;
    char retval = 0;
    char *ptr = (char*)p;
    if (position == NULL) position = pcm_data;
    while (size--) {
        if (position >= pcm_data + pcm_size) {
            // D("EOF in pcm");
            retval = 1;
            position = pcm_data;
            break;
        }
        *(ptr++) = *(position++);
    }
    return retval;
}

static char WEAtoolsReadDataFromOrg(void *p, size_t size) {
    static char *position = NULL;
    char retval = 0;
    char *ptr = (char*)p;
    if (position == NULL) position = org_data;
    while (size--) {
        if (position >= org_data + org_size) {
            D("EOF in header");
            retval = 1;
            position = org_data;
            break;
        }
        *(ptr++) = *(position++);
    }
    return retval;
}

static void WEAtoolsLoadWaveHeader() {
    static unsigned char buf[4];
    wave_header_t *h = &header.h.wave_header;

    // 1-4: RIFF
    WEAtoolsReadDataFromOrg(h->riff, 4);
    assert(!strncmp("RIFF", h->riff, 4), "RIFF descriptor not found");
    // 5-8: RIFF size
    WEAtoolsReadDataFromOrg(buf, 4);
    h->overall_size = BUF2NUM4(buf);
    // 9-12: WAVE
    WEAtoolsReadDataFromOrg(h->wave, 4);
    assert(!strncmp("WAVE", h->wave, 4), "WAVE chunk ID not found");
    // 13-16: fmt
    WEAtoolsReadDataFromOrg(h->fmt_chunk_marker, 4);
    assert(!strncmp("fmt", h->fmt_chunk_marker, 3), "fmt chunk format not found");
    // 17-20: format size
    WEAtoolsReadDataFromOrg(buf, 4);
    h->length_of_fmt = BUF2NUM4(buf);
    // 21-22: format type
    WEAtoolsReadDataFromOrg(buf, 2);
    h->format_type = BUF2NUM2(buf);
    assert(h->format_type == PCM, "Only supports PcM");
    // 23-24: num channels
    WEAtoolsReadDataFromOrg(buf, 2);
    h->channels = BUF2NUM2(buf);
    // 25-28: sample rate
    WEAtoolsReadDataFromOrg(buf, 4);
    h->sample_rate = BUF2NUM4(buf);
    // 29-32: byte rate
    WEAtoolsReadDataFromOrg(buf, 4);
    h->byterate = BUF2NUM4(buf);
    // 33-34: block align
    WEAtoolsReadDataFromOrg(buf, 2);
    h->block_align = BUF2NUM2(buf);
    // 35-36: bits per sample
    WEAtoolsReadDataFromOrg(buf, 2);
    h->bits_per_sample = BUF2NUM2(buf);
    // data
    do {
        // chunk id
        WEAtoolsReadDataFromOrg(h->data_chunk_header, sizeof(h->data_chunk_header));
        // size
        WEAtoolsReadDataFromOrg(buf, 4);
        h->data_size = BUF2NUM4(buf);

        if (strncmp("data", h->data_chunk_header, 4) != 0) {
            while (h->data_size--) WEAtoolsReadDataFromOrg(buf, 1);
        } else break;
    } while (1);

    header.data_size = h->data_size;
    header.channels = h->channels;
    header.data_rate = h->data_size;
    header.sample_rate = h->sample_rate;
    header.bits_per_sample = h->bits_per_sample;
    header.duration = (double)h->overall_size / h->byterate;
    header.num_samples = h->data_size * 8 / (h->channels * h->bits_per_sample);
    header.sample_size = h->channels * h->bits_per_sample;
    header.type = WAVE;
    assert(header.sample_size % header.channels == 0, "wrong sample size");
}

static void WEAtoolsLoadFile(char *filename) {
    FILE *f = fopen(filename, "rb");
    assert(f, "unable to load file: %s", filename);

    fseek(f, 0, SEEK_END);
    org_size = ftell(f);
    rewind(f);

    org_data = (char*)malloc(org_size + 1);
    assert(org_data != NULL, "unable to alloc memory");
    assert(fread(org_data, org_size, 1, f) == 1, "entire read fails");

    fclose(f);
}

static void WEAtoolsReadFileHeader(char *filename) {
    int i = 0;
    while (filename[i++] != 0);
    while (i > 0 && filename[--i] != '.');
    char *file_ext = filename + i + 1;
    
    if (strcmp(file_ext, "wav") == 0) {
        WEAtoolsLoadWaveHeader();
    } else if (strcmp(file_ext, "mp3") == 0) {
        header.type = MP3;
    } else if (strcmp(file_ext, "flac") == 0) {
        header.type = FLAC;
    } else if (strcmp(file_ext, "m4a") == 0) {
        header.type = ALAC;
    }
    // TODO
    assert(header.type == WAVE, "only supports wav")
}

static void WEAtoolsLoadPcmData() {
    pcm_size = header.data_size;
    pcm_data = (char*)malloc(header.data_size);
    WEAtoolsReadDataFromOrg(pcm_data, pcm_size);
}

void WEAtoolsInit(char *filename) {
    WEAtoolsLoadFile(filename);
    WEAtoolsReadFileHeader(filename);
    int rc, dir = 0;
    // open pcm device for playback
    rc = snd_pcm_open(&handler, "default", SND_PCM_STREAM_PLAYBACK, 0);
    assert(rc >= 0, "unable to open pcm device: %s", snd_strerror(rc));

    // allocate a hardware parameters object and fill it in with default values
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handler, params);
    snd_pcm_hw_params_set_access(handler, params, SND_PCM_ACCESS_RW_INTERLEAVED);
#ifdef LITTLE_ENDIAN
    if (header.bits_per_sample == 8) {
        snd_pcm_hw_params_set_format(handler, params, SND_PCM_FORMAT_S8);
    } else if (header.bits_per_sample == 16) {
        snd_pcm_hw_params_set_format(handler, params, SND_PCM_FORMAT_S16_LE);
    } else if (header.bits_per_sample == 24) {
        snd_pcm_hw_params_set_format(handler, params, SND_PCM_FORMAT_S24_LE);
    } else if (header.bits_per_sample == 32) {
        snd_pcm_hw_params_set_format(handler, params, SND_PCM_FORMAT_S32_LE);
    }
#else
    if (header.bits_per_sample == 8) {
        snd_pcm_hw_params_set_format(handler, params, SND_PCM_FORMAT_S8);
    } else if (header.bits_per_sample == 16) {
        snd_pcm_hw_params_set_format(handler, params, SND_PCM_FORMAT_S16_BE);
    } else if (header.bits_per_sample == 24) {
        snd_pcm_hw_params_set_format(handler, params, SND_PCM_FORMAT_S24_BE);
    } else if (header.bits_per_sample == 32) {
        snd_pcm_hw_params_set_format(handler, params, SND_PCM_FORMAT_S32_BE);
    }
#endif
    snd_pcm_hw_params_set_channels(handler, params, header.channels);
    snd_pcm_hw_params_set_rate(handler, params, header.sample_rate, dir);
    snd_pcm_hw_params_set_period_size(handler, params, header.sample_size, dir);

    // write the parameters to the driver
    rc = snd_pcm_hw_params(handler, params);
    assert(rc >= 0, "unable to set hw parameters: %s", snd_strerror(rc));

    WEAtoolsLoadPcmData();
    sem_init(&sem_bgm_start, 0, 0);
}

void WEAtoolsDestruct() {
    snd_pcm_drain(handler);
    snd_pcm_close(handler);

    sem_destroy(&sem_bgm_start);

    if (org_data) free(org_data);
    if (pcm_data) free(pcm_data);
}

void WEAtoolsPlay() {
    // use a buffer to hold one period
    size_t buffer_size = header.bits_per_sample / 8 * header.channels * header.sample_size;
    char *buffer = buffer = (char*)malloc(buffer_size);

    P(sem_bgm_start);
    while (1) {
        while (WEAtoolsReadDataFromPcm(buffer, buffer_size) == 0) {
            snd_pcm_writei(handler, buffer, header.sample_size);
            if (sig_exit) goto __EXIT;
        }
        if (!opts.bgm_loop) break;
    }
__EXIT:
    WEAtoolsDestruct();
    free(buffer);
}
