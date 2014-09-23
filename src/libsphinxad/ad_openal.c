#include <config.h>
#include <stdlib.h>
#include <stdio.h>

#include "ad.h"

ad_rec_t *
ad_open_dev(const char * dev, int32 samples_per_sec)
{
    ad_rec_t * handle = malloc(sizeof(ad_rec_t));

    if (handle == NULL) {
        fprintf(stderr, "%s\n", "failed to allocate memory");
        abort();
    }

    handle -> device = alcCaptureOpenDevice(dev, samples_per_sec, AL_FORMAT_MONO16, samples_per_sec * 10);

    if (handle -> device == NULL) {
        free(handle);
        fprintf(stderr, "%s\n", "failed to open capture device");
        abort();
    }

    return handle;
}


ad_rec_t *
ad_open_sps(int32 samples_per_sec)
{
    return ad_open_dev(NULL, samples_per_sec);
}

ad_rec_t *
ad_open(void)
{
    return ad_open_sps(DEFAULT_SAMPLES_PER_SEC);
}


int32
ad_start_rec(ad_rec_t * r)
{
    alcCaptureStart(r -> device);
    return 0;
}


int32
ad_stop_rec(ad_rec_t * r)
{
    alcCaptureStop(r -> device);
    return 0;
}


int32
ad_read(ad_rec_t * r, int16 * buf, int32 max)
{
    ALCint number;

    alcGetIntegerv(r -> device, ALC_CAPTURE_SAMPLES, sizeof(number), &number);
    if (number >= 0) {
        number = (number < max ? number : max);
        alcCaptureSamples(r -> device, buf, number);
    }

    return number;
}


int32
ad_close(ad_rec_t * r)
{
    ALCboolean isClosed;

    isClosed = alcCaptureCloseDevice(r -> device);

    if (isClosed) {
        return 0;
    } else {
        return -1;
    }
}
