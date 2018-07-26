/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <alloca.h>

#include <linux/input.h>
#include <linux/joystick.h>

#include "af-steering-wheel-binding.h"
#include "js_raw.h"
#include "js_signal_event.h"

#define JSNAMELEN 128

static int *axis;
static char *button;

int js_signal_read(int fd)
{
	ssize_t size;
	struct js_event jsEvent;
	int rc = 0;

	size = read(fd, &jsEvent, sizeof(struct js_event));
	if(size != 0)
	{
		switch (jsEvent.type & ~JS_EVENT_INIT)
		{
			case JS_EVENT_BUTTON:
				button[jsEvent.number] = (char)jsEvent.value;
				//NOTICEMSG("JS_EVENT_BUTTON %d\n", button[jsEvent.number]);
				newButtonValue(jsEvent.number, jsEvent.value);
				break;
			case JS_EVENT_AXIS:
				axis[jsEvent.number] = jsEvent.value;
				//NOTICEMSG("JS_EVENT_AXIS %d\n", axis[jsEvent.number]);
				newAxisValue(jsEvent.number, jsEvent.value);
				break;
			default:
				break;
		}

		rc = 0;
	}
	else
	{
		rc = -1;
	}

	return rc;
}

int on_event(sd_event_source *s, int fd, uint32_t revents, void *userdata)
{
	if ((revents & EPOLLIN) != 0)
	{
		//NOTICEMSG("on_event!\n");
		int rc = 0;

		rc = js_signal_read(fd);
		if(rc == -1)
		{
			ERRMSG("JS Frame Read failed");

			return -1;
		}
	}
	if ((revents & (EPOLLERR|EPOLLRDHUP|EPOLLHUP)) != 0)
	{
		/* T.B.D
		 * if error or hungup */
		ERRMSG("Error or Hunup: rvent=%08x", revents);
	}

	return 0;
}

int  js_open(const char *devname)
{
	unsigned char numAxes = 0;
	unsigned char numButtons = 0;
	int version = 0;
	int fd;
	char name[JSNAMELEN] = "Unknown";

	struct js_corr cal[6];
	int i, j;
	unsigned int calData[36] =
	{
		1, 0, 8191, 8192, 65542, 65534,
		1, 0, 127, 128, 4227201, 4194176,
		1, 0, 127, 128, 4227201, 4194176,
		1, 0, 127, 128, 4227201, 4194176,
		1, 0, 0, 0, 536854528, 536854528,
		1, 0, 0, 0, 536854528, 536854528
	};

	if ((fd = open(devname, O_RDONLY)) < 0)
	{
		return -1;
	}

	ioctl(fd, JSIOCGVERSION, &version);
	ioctl(fd, JSIOCGAXES, &numAxes);
	ioctl(fd, JSIOCGBUTTONS, &numButtons);
	ioctl(fd, JSIOCGNAME(JSNAMELEN), name);

	for (i = 0; i < 6; i++)
	{
		int k = 0;
		cal[i].type = (__u16)calData[(i*6)+k];
		k++;
		cal[i].prec = (__s16)calData[(i*6)+k];
		k++;

		for(j = 0; j < 4; j++)
		{
			cal[i].coef[j] = (__s32)calData[(i*6)+k];
			k++;
		}
	}

	if (ioctl(fd, JSIOCSCORR, &cal) < 0)
	{
		return -1;
	}

	axis = (int *)calloc(numAxes, sizeof(int));
	button = (char *)calloc(numButtons, sizeof(char));
#if 0
	gis = g_unix_input_stream_new(fd, TRUE);
	if(gis == NULL)
	{
		ERRMSG("g_unix_input_stream_new() failed!");
	}
	else
	{
		NOTICEMSG("g_unix_input_stream_new() succeed!");
	}
	g_input_stream_read_async(gis, &jsEvent, sizeof(struct js_event), G_PRIORITY_DEFAULT, NULL, &readCallback, NULL);
#endif

	return fd;
}

void js_close(int js)
{
	if (js < 0)
	{
		return;
	}

	close(js);
}
