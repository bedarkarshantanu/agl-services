/*
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2016, 2017 Konsulko Group
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RTL_FM_H
#define RTL_FM_H

#include <stdint.h>

#define RTL_FM_DEFAULT_BUF_LENGTH	(1 * 16384)
#define RTL_FM_MAXIMUM_OVERSAMPLE	16
#define RTL_FM_MAXIMUM_BUF_LENGTH	(RTL_FM_MAXIMUM_OVERSAMPLE * RTL_FM_DEFAULT_BUF_LENGTH)

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*rtl_fm_output_fn_t)(int16_t *result, int result_len, void *data);

int rtl_fm_init(uint32_t freq,
		uint32_t sample_rate,
		uint32_t resample_rate,
		rtl_fm_output_fn_t output_fn,
		void *output_fn_data);

void rtl_fm_start(void);

void rtl_fm_set_freq(uint32_t freq);

void rtl_fm_set_freq_callback(void (*callback)(uint32_t, void *),
			      void *data);

uint32_t rtl_fm_get_freq(void);

void rtl_fm_stop(void);

void rtl_fm_scan_start(int direction,
		       void (*callback)(uint32_t, void *),
		       void *data,
		       uint32_t step,
		       uint32_t min,
		       uint32_t max);

void rtl_fm_scan_stop(void);

void rtl_fm_scan_set_squelch_level(int level);

void rtl_fm_scan_set_squelch_limit(int count);

void rtl_fm_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* RTL_FM_H */
