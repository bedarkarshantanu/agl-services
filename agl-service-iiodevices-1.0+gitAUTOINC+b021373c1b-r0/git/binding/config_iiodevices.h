#ifndef _CONFIG_IIODEVICES_
#define _CONFIG_IIODEVICES_

#include <stdio.h>
#include <stdlib.h>

#define IIODEVICE "/sys/bus/iio/devices/"

struct iio_info {
    const char *dev_name;
    const char *id;
    const char *middlename;
};

enum iio_elements { X = 1, Y = 2, Z = 4 };

void set_channel_name(char *name, enum iio_elements iioelts);
enum iio_elements treat_iio_elts(const char *iioelts_string);
int get_iio_nb(enum iio_elements iioelts);

#endif //_CONFIG_IIODEVICES_
