/*
 * Copyright 2018 Konsulko Group
 * Author: Pantelis Antoniou <pantelis.antoniou@konsulko.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include <glib.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <glib-object.h>

#include <json-c/json.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

#include "network-api.h"
#include "network-common.h"

G_DEFINE_QUARK(network-binding-error-quark, nb_error)

/* convert dbus key to lower case */
gboolean auto_lowercase_keys = TRUE;

int str2boolean(const char *value)
{
	if (!strcmp(value, "1") || !g_ascii_strcasecmp(value, "true") ||
	    !g_ascii_strcasecmp(value, "on") || !g_ascii_strcasecmp(value, "enabled") ||
	    !g_ascii_strcasecmp(value, "yes"))
		return TRUE;
	if (!strcmp(value, "0") || !g_ascii_strcasecmp(value, "false") ||
		 !g_ascii_strcasecmp(value, "off") || !g_ascii_strcasecmp(value, "disabled") ||
		 !g_ascii_strcasecmp(value, "no"))
		return FALSE;
	return -1;
}

json_object *json_object_copy(json_object *jval)
{
	json_object *jobj;
	int i, len;

	/* handle NULL */
	if (!jval)
		return NULL;

	switch (json_object_get_type(jval)) {
	case json_type_object:
		jobj = json_object_new_object();
		json_object_object_foreach(jval, key, jval2)
			json_object_object_add(jobj, key,
					json_object_copy(jval2));

		return jobj;

	case json_type_array:
		jobj = json_object_new_array();
		len = json_object_array_length(jval);
		for (i = 0; i < len; i++)
			json_object_array_add(jobj,
				json_object_copy(
					json_object_array_get_idx(jval, i)));
		return jobj;

	case json_type_null:
		return NULL;

	case json_type_boolean:
		return json_object_new_boolean(
				json_object_get_boolean(jval));

	case json_type_double:
		return json_object_new_double(
				json_object_get_double(jval));

	case json_type_int:
		return json_object_new_int64(
				json_object_get_int64(jval));

	case json_type_string:
		return json_object_new_string(
				json_object_get_string(jval));
	}

	g_assert(0);
	/* should never happen */
	return NULL;
}

gchar *key_dbus_to_json(const gchar *key, gboolean auto_lower)
{
	gchar *lower, *s;

	lower = g_strdup(key);
	g_assert(lower);

	if (!auto_lower)
		return lower;

	/* convert to lower case */
	for (s = lower; *s; s++)
		*s = g_ascii_tolower(*s);

	return lower;
}

json_object *simple_gvariant_to_json(GVariant *var, json_object *parent,
		gboolean recurse)
{
	json_object *obj = NULL, *item;
	gint32 i32;
	gint64 i64;
	guint32 ui32;
	guint64 ui64;
	GVariantIter iter;
	GVariant *key, *value;
	gchar *json_key;
	gsize nitems;
	gboolean is_dict;

	obj = NULL;

	/* AFB_DEBUG("g_variant_classify(var)=%c", g_variant_classify(var)); */

	/* we only handle simple types */
	switch (g_variant_classify(var)) {
	case G_VARIANT_CLASS_BOOLEAN:
		obj = json_object_new_boolean(g_variant_get_boolean(var));
		break;
	case G_VARIANT_CLASS_INT16:
		obj = json_object_new_int(g_variant_get_int16(var));
		break;
	case G_VARIANT_CLASS_INT32:
		i32 = g_variant_get_int32(var);
		obj = json_object_new_int(i32);
		break;
	case G_VARIANT_CLASS_INT64:
		i64 = g_variant_get_int64(var);
		if (i64 >= -(1L << 31) && i64 < (1L << 31))
			obj = json_object_new_int((int)i64);
		else
			obj = json_object_new_int64(i64);
		break;
	case G_VARIANT_CLASS_BYTE:
		obj = json_object_new_int((int)g_variant_get_byte(var));
		break;
	case G_VARIANT_CLASS_UINT16:
		obj = json_object_new_int((int)g_variant_get_uint16(var));
		break;
	case G_VARIANT_CLASS_UINT32:
		ui32 = g_variant_get_uint32(var);
		if (ui32 < (1U << 31))
			obj = json_object_new_int(ui32);
		else
			obj = json_object_new_int64(ui32);
		break;
	case G_VARIANT_CLASS_UINT64:
		ui64 = g_variant_get_uint64(var);
		if (ui64 < (1U << 31))
			obj = json_object_new_int((int)ui64);
		else if (ui64 < (1LLU << 63))
			obj = json_object_new_int64(ui64);
		else {
			AFB_WARNING("U64 value %llu clamped to %llu",
					(unsigned long long)ui64,
					(unsigned long long)((1LLU << 63) - 1));
			obj = json_object_new_int64((1LLU << 63) - 1);
		}
		break;
	case G_VARIANT_CLASS_DOUBLE:
		obj = json_object_new_double(g_variant_get_double(var));
		break;
	case G_VARIANT_CLASS_STRING:
		obj = json_object_new_string(g_variant_get_string(var, NULL));
		break;

	case G_VARIANT_CLASS_ARRAY:

		if (!recurse)
			break;

		/* detect dictionaries which are arrays of dict entries */
		g_variant_iter_init(&iter, var);

		nitems = g_variant_iter_n_children(&iter);
		/* remove completely empty arrays */
		if (nitems == 0)
			break;

		is_dict = nitems > 0;
		while (is_dict && (value = g_variant_iter_next_value(&iter))) {
			is_dict = g_variant_classify(value) == G_VARIANT_CLASS_DICT_ENTRY;
			g_variant_unref(value);
		}

		if (is_dict)
			obj = json_object_new_object();
		else
			obj = json_object_new_array();

		g_variant_iter_init(&iter, var);
		while ((value = g_variant_iter_next_value(&iter))) {

			item = simple_gvariant_to_json(value, obj, TRUE);
			if (!is_dict && item)
				json_object_array_add(obj, item);

			g_variant_unref(value);
		}
		break;

	case G_VARIANT_CLASS_DICT_ENTRY:

		if (!recurse)
			break;

		if (!parent) {
			AFB_WARNING("#### dict new object without a parent");
			break;
		}

		g_variant_iter_init(&iter, var);
		while ((key = g_variant_iter_next_value(&iter))) {

			value = g_variant_iter_next_value(&iter);
			if (!value) {
				AFB_WARNING("Out of values with a key");
				g_variant_unref(key);
				break;
			}

			json_key = key_dbus_to_json(
					g_variant_get_string(key, NULL),
					auto_lowercase_keys);

			/* only handle dict values with string keys */
			if (g_variant_classify(key) == G_VARIANT_CLASS_STRING) {
				item = simple_gvariant_to_json(value, obj, TRUE);
				if (item)
					json_object_object_add(parent, json_key, item);

			} else
				AFB_WARNING("Can't handle non-string key");

			g_free(json_key);

			g_variant_unref(value);
			g_variant_unref(key);
		}
		break;

	case G_VARIANT_CLASS_VARIANT:

		/* NOTE: recurse allowed because we only allow a single encapsulated variant */

		g_variant_iter_init(&iter, var);
		nitems = g_variant_iter_n_children(&iter);
		if (nitems != 1) {
			AFB_WARNING("Can't handle variants with more than one children (%lu)", nitems);
			break;
		}

		while ((value = g_variant_iter_next_value(&iter))) {
			obj = simple_gvariant_to_json(value, parent, TRUE);
			g_variant_unref(value);
			break;
		}
		break;

	default:
		AFB_WARNING("############ class is %c", g_variant_classify(var));
		obj = NULL;
		break;
	}

	return obj;
}

gchar *property_name_dbus2json(const struct property_info *pi,
		gboolean is_config)
{
	gchar *json_name;
	gchar *cfgname;

	if (pi->json_name)
		json_name = g_strdup(pi->json_name);
	else
		json_name = key_dbus_to_json(pi->name, auto_lowercase_keys);

	if (!json_name)
		return NULL;

	if (!is_config)
		return json_name;

	cfgname = g_strdup_printf("%s.%configuration",
			json_name,
			auto_lowercase_keys ? 'c' : 'C');
	g_free(json_name);
	return cfgname;
}

json_object *property_dbus2json(
		const struct property_info **pip,
		const gchar *key, GVariant *var,
		gboolean *is_config)
{
	const struct property_info *pi = *pip, *pi2, *pi_sub;
	GVariantIter iter, iter2;
	json_object *obj = NULL, *obji;
	const char *fmt;
	GVariant *value, *dict_value, *dict_key;
	const gchar *sub_key;
	gchar *json_key;
	gboolean is_subconfig;

	if (key) {
		pi = property_by_dbus_name(pi, key, is_config);
		if (!pi)
			return NULL;
		*pip = pi;
	}

	fmt = pi->fmt;

	obj = simple_gvariant_to_json(var, NULL, FALSE);
	if (obj) {
		/* TODO check fmt for matching type */
		return obj;
	}

	switch (*fmt) {
	case 'a':	/* array */
		obj = json_object_new_array();

		g_variant_iter_init(&iter, var);
		while ((value = g_variant_iter_next_value(&iter))) {
			pi2 = pi;
			obji = property_dbus2json(&pi2, NULL, value,
					&is_subconfig);
			if (obji)
				json_object_array_add(obj, obji);

			g_variant_unref(value);
		}
		break;
	case '{':
		/* we only support {sX} */

		/* there must be a sub property entry */
		g_assert(pi->sub);

		obj = json_object_new_object();

		g_variant_iter_init(&iter, var);
		while ((value = g_variant_iter_next_value(&iter))) {

			if (g_variant_classify(value) != G_VARIANT_CLASS_DICT_ENTRY) {
				AFB_WARNING("Expecting dict got '%c'", g_variant_classify(value));
				g_variant_unref(value);
				break;
			}

			g_variant_iter_init(&iter2, value);
			while ((dict_key = g_variant_iter_next_value(&iter2))) {
				if (g_variant_classify(dict_key) != G_VARIANT_CLASS_STRING) {
					AFB_WARNING("Can't handle non-string dict keys '%c'",
							g_variant_classify(dict_key));
					g_variant_unref(dict_key);
					g_variant_unref(value);
					continue;
				}

				dict_value = g_variant_iter_next_value(&iter2);
				if (!dict_value) {
					AFB_WARNING("Out of values with a dict_key");
					g_variant_unref(dict_key);
					g_variant_unref(value);
					break;
				}

				sub_key = g_variant_get_string(dict_key, NULL);

				pi_sub = pi->sub;
				while (pi_sub->name) {
					if (!g_strcmp0(sub_key, pi_sub->name))
						break;
					pi_sub++;
				}

				if (pi_sub->name) {
					pi2 = pi_sub;
					obji = property_dbus2json(&pi2,
							sub_key, dict_value,
							&is_subconfig);
					if (obji) {
						json_key = property_name_dbus2json(pi2, FALSE);
						json_object_object_add(obj, json_key, obji);
						g_free(json_key);
					}
				} else
					AFB_INFO("Unhandled %s/%s property", key, sub_key);

				g_variant_unref(dict_value);
				g_variant_unref(dict_key);
			}

			g_variant_unref(value);
		}

		break;
	}

	if (!obj)
		AFB_INFO("# %s not a type we can handle", key ? key : "<NULL>");

	return obj;
}

const struct property_info *property_by_dbus_name(
		const struct property_info *pi,
		const gchar *dbus_name,
		gboolean *is_config)
{
	const struct property_info *pit;
	const gchar *suffix;
	gchar *tmpname;
	size_t len;

	/* direct match first */
	pit = pi;
	while (pit->name) {
		if (!g_strcmp0(dbus_name, pit->name)) {
			if (is_config)
				*is_config = FALSE;
			return pit;
		}
		pit++;
	}

	/* try to see if a matching config property exists */
	suffix = strrchr(dbus_name, '.');
	if (!suffix || g_ascii_strcasecmp(suffix, ".Configuration"))
		return NULL;

	/* it's a (possible) .config property */
	len = suffix - dbus_name;
	tmpname = alloca(len + 1);
	memcpy(tmpname, dbus_name, len);
	tmpname[len] = '\0';

	/* match with config */
	pit = pi;
	while (pit->name) {
		if (!(pit->flags & PI_CONFIG)) {
			pit++;
			continue;
		}
		if (!g_strcmp0(tmpname, pit->name)) {
			if (is_config)
				*is_config = TRUE;
			return pit;
		}
		pit++;
	}

	return NULL;
}

const struct property_info *property_by_json_name(
		const struct property_info *pi,
		const gchar *json_name,
		gboolean *is_config)
{
	const struct property_info *pit;
	gchar *this_json_name;
	const gchar *suffix;
	gchar *tmpname;
	size_t len;

	/* direct match */
	pit = pi;
	while (pit->name) {
		this_json_name = property_name_dbus2json(pit, FALSE);
		if (!g_strcmp0(this_json_name, json_name)) {
			g_free(this_json_name);
			if (is_config)
				*is_config = FALSE;
			return pit;
		}
		g_free(this_json_name);
		pit++;
	}

	/* try to see if a matching config property exists */
	suffix = strrchr(json_name, '.');
	if (!suffix || g_ascii_strcasecmp(suffix, ".configuration"))
		return NULL;

	/* it's a (possible) .config property */
	len = suffix - json_name;
	tmpname = alloca(len + 1);
	memcpy(tmpname, json_name, len);
	tmpname[len] = '\0';

	/* match with config */
	pit = pi;
	while (pit->name) {
		if (!(pit->flags & PI_CONFIG)) {
			pit++;
			continue;
		}
		this_json_name = property_name_dbus2json(pit, FALSE);
		if (!g_strcmp0(this_json_name, tmpname)) {
			g_free(this_json_name);
			if (is_config)
				*is_config = TRUE;
			return pit;
		}
		g_free(this_json_name);
		pit++;
	}

	return NULL;
}

const struct property_info *property_by_name(
		const struct property_info *pi,
		gboolean is_json_name, const gchar *name,
		gboolean *is_config)
{
	return is_json_name ?
		property_by_json_name(pi, name, is_config) :
		property_by_dbus_name(pi, name, is_config);
}

gchar *property_get_json_name(const struct property_info *pi,
		const gchar *name)
{
	gboolean is_config;

	pi = property_by_dbus_name(pi, name, &is_config);
	if (!pi)
		return NULL;
	return property_name_dbus2json(pi, is_config);
}

gchar *configuration_dbus_name(const gchar *dbus_name)
{
	return g_strdup_printf("%s.Configuration", dbus_name);
}

gchar *configuration_json_name(const gchar *json_name)
{
	return g_strdup_printf("%s.configuration", json_name);
}

gchar *property_get_name(const struct property_info *pi,
		const gchar *json_name)
{
	gboolean is_config;

	pi = property_by_json_name(pi, json_name, &is_config);
	if (!pi)
		return NULL;

	return !is_config ? g_strdup(pi->name) :
		configuration_dbus_name(pi->name);
}

gboolean root_property_dbus2json(
		json_object *jparent,
		const struct property_info *pi,
		const gchar *key, GVariant *var,
		gboolean *is_config)
{
	json_object *obj;
	gchar *json_key;

	obj = property_dbus2json(&pi, key, var, is_config);
	if (!obj)
		return FALSE;

	switch (json_object_get_type(jparent)) {
	case json_type_object:
		json_key = property_name_dbus2json(pi, *is_config);
		json_object_object_add(jparent, json_key, obj);
		g_free(json_key);
		break;
	case json_type_array:
		json_object_array_add(jparent, obj);
		break;
	default:
		json_object_put(obj);
		return FALSE;
	}

	return TRUE;
}

static const GVariantType *type_from_fmt(const char *fmt)
{
	switch (*fmt) {
	case 'b':	/* gboolean */
		return G_VARIANT_TYPE_BOOLEAN;
	case 'y':	/* guchar */
		return G_VARIANT_TYPE_BYTE;
	case 'n':	/* gint16 */
		return G_VARIANT_TYPE_INT16;
	case 'q':	/* guint16 */
		return G_VARIANT_TYPE_UINT16;
	case 'h':
		return G_VARIANT_TYPE_HANDLE;
	case 'i':	/* gint32 */
		return G_VARIANT_TYPE_INT32;
	case 'u':	/* guint32 */
		return G_VARIANT_TYPE_UINT32;
	case 'x':	/* gint64 */
		return G_VARIANT_TYPE_INT64;
	case 't':	/* gint64 */
		return G_VARIANT_TYPE_UINT64;
	case 'd':	/* double */
		return G_VARIANT_TYPE_DOUBLE;
	case 's':	/* string */
		return G_VARIANT_TYPE_STRING;
	case 'o':	/* object */
		return G_VARIANT_TYPE_OBJECT_PATH;
	case 'g':	/* signature */
		return G_VARIANT_TYPE_SIGNATURE;
	case 'v':	/* variant */
		return G_VARIANT_TYPE_VARIANT;
	}
	/* nothing complex */
	return NULL;
}

GVariant *property_json_to_gvariant(
		const struct property_info *pi,
		const char *fmt,
		const struct property_info *pi_parent,
		json_object *jval,
		GError **error)
{
	const struct property_info *pi_sub;
	GVariant *arg, *item;
	GVariantBuilder builder;
	json_object *jitem;
	json_bool b;
	gchar *dbus_name;
	int64_t i64;
	double d;
	const char *jvalstr, *str;
	char c;
	int i, len;
	gboolean is_config;

	if (!fmt)
		fmt = pi->fmt;

	if (!jval) {
		g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
			"can't encode json NULL type");
		return NULL;
	}

	jvalstr = json_object_to_json_string(jval);

	arg = NULL;

	b = FALSE;
	i64 = 0;
	d = 0.0;
	str = NULL;

	/* conversion and type check */
	c = *fmt++;
	if (c == 'a') {
		if (!json_object_is_type(jval, json_type_array)) {
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Bad property \"%s\" (not an array)",
					jvalstr);
			return NULL;
		}

		len = json_object_array_length(jval);
		/* special case for empty array */
		if (!len) {
			arg = g_variant_new_array(type_from_fmt(fmt), NULL, 0);
			if (!arg)
				g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
					"Can't create empty array on \"%s\"",
						jvalstr);
			return arg;
		}

		g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
		for (i = 0; i < len; i++) {
			jitem = json_object_array_get_idx(jval, i);
			item = property_json_to_gvariant(pi, fmt, NULL, jitem, error);
			if (!item) {
				g_variant_builder_clear(&builder);
				return NULL;
			}
			g_variant_builder_add_value(&builder, item);
		}

		arg = g_variant_builder_end(&builder);

		if (!arg)
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Can't handle array on \"%s\"",
					jvalstr);

		return arg;
	}
	if (c == '{') {
		g_assert(pi->sub);

		c = *fmt++;
		/* we only handle string keys */
		if (c != 's') {
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Can't handle non-string keys on \"%s\"",
					jvalstr);
			return NULL;
		}
		c = *fmt++;

		/* this is arguably wrong */
		g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
		pi_sub = pi->sub;
		json_object_object_foreach(jval, key_o, jval_o) {
			pi_sub = property_by_json_name(pi->sub, key_o, &is_config);
			if (!pi_sub) {
				g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
					"Unknown sub-property %s in \"%s\"",
						key_o, json_object_to_json_string(jval_o));
				return NULL;
			}
			item = property_json_to_gvariant(pi_sub, NULL, pi, jval_o, error);
			if (!item)
				return NULL;

			dbus_name = property_get_name(pi->sub, key_o);
			g_assert(dbus_name);	/* can't fail; but check */

			g_variant_builder_add(&builder, pi->fmt, dbus_name, item);

			g_free(dbus_name);
		}

		arg = g_variant_builder_end(&builder);

		if (!arg)
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Can't handle object on \"%s\"",
					jvalstr);
		return arg;
	}

	switch (c) {
	case 'b':	/* gboolean */
		if (!json_object_is_type(jval, json_type_boolean)) {
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Bad property \"%s\" (not a boolean)",
					jvalstr);
			return NULL;
		}
		b = json_object_get_boolean(jval);
		break;
	case 'y':	/* guchar */
	case 'n':	/* gint16 */
	case 'q':	/* guint16 */
	case 'h':
	case 'i':	/* gint32 */
	case 'u':	/* guint32 */
	case 'x':	/* gint64 */
	case 't':	/* gint64 */
		if (!json_object_is_type(jval, json_type_int)) {
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Bad property \"%s\" (not an integer)",
					jvalstr);
			return NULL;
		}
		/* note unsigned 64 bit values shall be truncated */
		i64 = json_object_get_int64(jval);
		break;

	case 'd':	/* double */
		if (!json_object_is_type(jval, json_type_double)) {
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Bad property \"%s\" (not a double)",
					jvalstr);
			return NULL;
		}
		d = json_object_get_double(jval);
		break;
	case 's':	/* string */
	case 'o':	/* object */
	case 'g':	/* signature */
		if (!json_object_is_type(jval, json_type_string)) {
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Bad property \"%s\" (not a string)",
					jvalstr);
			return NULL;
		}
		str = json_object_get_string(jval);
		break;
	case 'v':	/* variant */
		AFB_WARNING("Can't handle variant yet");
		break;
	}

	/* build gvariant */
	switch (c) {
	case 'b':	/* gboolean */
		arg = g_variant_new_boolean(b);
		break;
	case 'y':	/* guchar */
		if (i64 < 0 || i64 > 255) {
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Bad property %s (out of byte range)",
					jvalstr);
			return FALSE;
		}
		arg = g_variant_new_byte((guchar)i64);
		break;
	case 'n':	/* gint16 */
		if (i64 < -(1LL << 15) || i64 >= (1LL << 15)) {
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Bad property %s (out of int16 range)",
					jvalstr);
			return FALSE;
		}
		arg = g_variant_new_int16((gint16)i64);
		break;
	case 'q':	/* guint16 */
		if (i64 < 0 || i64 >= (1LL << 16)) {
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Bad property %s (out of uint16 range)",
					jvalstr);
			return FALSE;
		}
		arg = g_variant_new_uint16((guint16)i64);
		break;
	case 'h':
	case 'i':	/* gint32 */
		if (i64 < -(1LL << 31) || i64 >= (1LL << 31)) {
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Bad property %s (out of int32 range)",
					jvalstr);
			return FALSE;
		}
		arg = g_variant_new_int32((gint32)i64);
		break;
	case 'u':	/* guint32 */
		if (i64 < 0 || i64 >= (1LL << 32)) {
			g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Bad property %s (out of uint32 range)",
					jvalstr);
			return FALSE;
		}
		arg = g_variant_new_uint32((guint32)i64);
		break;
	case 'x':	/* gint64 */
		arg = g_variant_new_int64(i64);
		break;
	case 't':	/* gint64 */
		arg = g_variant_new_uint64(i64);
		break;
	case 'd':	/* double */
		arg = g_variant_new_double(d);
		break;
	case 's':	/* string */
		arg = g_variant_new_string(str);
		break;
	case 'o':	/* object */
		arg = g_variant_new_object_path(str);
		break;
	case 'g':	/* signature */
		arg = g_variant_new_signature(str);
		break;
	case 'v':	/* variant */
		AFB_WARNING("Can't handle variant yet");
		break;
	}

	return arg;
}

json_object *get_property_collect(json_object *jreqprop, json_object *jprop,
		GError **error)
{
	int i, len;
	json_object *jkey, *jval, *jobj = NULL, *jobjval;
	const char *key;


	/* printf("jreqprop=%s\n", json_object_to_json_string_ext(jreqprop,
					JSON_C_TO_STRING_SPACED));
	printf("jprop=%s\n", json_object_to_json_string_ext(jprop,
					JSON_C_TO_STRING_SPACED)); */

	/* get is an array of strings (or an object for subtype */
	g_assert(json_object_get_type(jreqprop) == json_type_array);

	len = json_object_array_length(jreqprop);
	if (len == 0)
		return NULL;

	for (i = 0; i < len; i++) {
		jkey = json_object_array_get_idx(jreqprop, i);

		/* string key */
		if (json_object_is_type(jkey, json_type_string)) {
			key = json_object_get_string(jkey);
			if (!json_object_object_get_ex(jprop, key, &jval)) {
				g_set_error(error,
					NB_ERROR, NB_ERROR_BAD_PROPERTY,
					"can't find key %s", key);
				json_object_put(jobj);
				return NULL;
			}

			if (!jobj)
				jobj = json_object_new_object();

			json_object_object_add(jobj, key,
					json_object_copy(jval));

		} else if (json_object_is_type(jkey, json_type_object)) {
			/* recursing into an object */

			json_object_object_foreach(jkey, key_o, jval_o) {
				if (!json_object_object_get_ex(jprop, key_o,
							&jval)) {
					g_set_error(error, NB_ERROR,
						NB_ERROR_BAD_PROPERTY,
						"can't find key %s", key_o);
					json_object_put(jobj);
					return NULL;
				}

				/* jval_o is on jreqprop */
				/* jval is on jprop */

				jobjval = get_property_collect(jval_o, jval,
						error);

				if (!jobjval && error && *error) {
					json_object_put(jobj);
					return NULL;
				}

				if (jobjval) {
					if (!jobj)
						jobj = json_object_new_object();

					json_object_object_add(jobj, key_o, jobjval);
				}
			}
		}
	}

	/* if (jobj)
		printf("jobj=%s\n", json_object_to_json_string_ext(jobj,
					JSON_C_TO_STRING_SPACED)); */

	return jobj;
}

json_object *get_named_property(const struct property_info *pi,
		gboolean is_json_name, const char *name, json_object *jprop)
{
	json_object *jret = NULL;
	gchar *json_name = NULL;

	if (!is_json_name) {
		json_name = property_get_json_name(pi, name);
		if (!json_name)
			return NULL;
		name = json_name;
	}

	json_object_object_foreach(jprop, key, jval) {
		if (!g_strcmp0(key, name)) {
			jret = json_object_copy(jval);
			break;
		}
	}

	g_free(json_name);

	return jret;
}
