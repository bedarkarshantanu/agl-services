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

static const struct property_info manager_props[] = {
	{ .name = "State", 		.fmt = "s", },
	{ .name = "OfflineMode",	.fmt = "b", },
	{ .name = "SessionMode",	.fmt = "b", },
	{ },
};

static const struct property_info technology_props[] = {
	{ .name = "Name", 		.fmt = "s", },
	{ .name = "Type", 		.fmt = "s", },
	{ .name = "Powered", 		.fmt = "b", },
	{ .name = "Connected",		.fmt = "b", },
	{ .name = "Tethering",		.fmt = "b", },
	{ .name = "TetheringIdentifier",.fmt = "s", },
	{ .name = "TetheringPassphrase",.fmt = "s", },
	{ },
};

static const struct property_info service_props[] = {
	/* simple types */
	{ .name = "State",		.fmt = "s", },
	{ .name = "Error",		.fmt = "s", },
	{ .name = "Type",		.fmt = "s", },
	{ .name = "Name",		.fmt = "s", },
	{ .name = "Favorite",		.fmt = "b", },
	{ .name = "Immutable",		.fmt = "b", },
	{ .name = "AutoConnect",	.fmt = "b", },
	{ .name = "Strength",		.fmt = "y", },
	{ .name = "Security",		.fmt = "as", },
	{ .name = "Roaming",		.fmt = "b", },

	/* complex types with configuration (but no subtype) */
	{
		.name	= "Nameservers",
		.fmt	= "as",
		.flags	= PI_CONFIG,
	}, {
		.name	= "Timeservers",
		.fmt	= "as",
		.flags	= PI_CONFIG,
	}, {
		.name	= "Domains",
		.fmt	= "as",
		.flags	= PI_CONFIG,
	}, {
		.name	= "mDNS",
		.fmt	= "b",
		.flags	= PI_CONFIG,
	},

	/* complex types with subtypes */
	{
		.name	= "IPv4",
		.fmt	= "{sv}",
		.flags	= PI_CONFIG,
		.sub	= (const struct property_info []) {
			{ .name = "Method",		.fmt = "s", },
			{ .name = "Address",		.fmt = "s", },
			{ .name = "Netmask",		.fmt = "s", },
			{ .name = "Gateway",		.fmt = "s", },
			{ },
		},
	}, {
		.name	= "IPv6",
		.fmt	= "{sv}",
		.flags	= PI_CONFIG,
		.sub	= (const struct property_info []) {
			{ .name = "Method",		.fmt = "s", },
			{ .name = "Address",		.fmt = "s", },
			{ .name = "PrefixLength",	.fmt = "y", },
			{ .name = "Gateway",		.fmt = "s", },
			{ .name = "Privacy",		.fmt = "s", },
			{ },
		},
	}, {
		.name	= "Proxy",
		.fmt	= "{sv}",
		.flags	= PI_CONFIG,
		.sub	= (const struct property_info []) {
			{ .name = "Method",		.fmt = "s", },
			{ .name = "URL",		.fmt = "s", },
			{ .name = "Servers",		.fmt = "as", },
			{ .name = "Excludes",		.fmt = "as", },
			{ },
		},
	}, {
		.name	= "Ethernet",
		.fmt	= "{sv}",
		.sub	= (const struct property_info []) {
			{ .name = "Method",		.fmt = "s", },
			{ .name = "Interface",		.fmt = "s", },
			{ .name = "Address",		.fmt = "s", },
			{ .name = "MTU",		.fmt = "q", },
			{ },
		},
	}, {
		.name	= "Provider",
		.fmt	= "{sv}",
		.flags	= PI_CONFIG,
		.sub	= (const struct property_info []) {
			{ .name = "Host",		.fmt = "s", },
			{ .name = "Domain",		.fmt = "s", },
			{ .name = "Name",		.fmt = "s", },
			{ .name = "Type",		.fmt = "s", },
			{ },
		},
	},
	{ }
};

const struct property_info *connman_get_property_info(
		const char *access_type, GError **error)
{
	const struct property_info *pi = NULL;

	if (!strcmp(access_type, CONNMAN_AT_MANAGER))
		pi = manager_props;
	else if (!strcmp(access_type, CONNMAN_AT_TECHNOLOGY))
		pi = technology_props;
	else if (!strcmp(access_type, CONNMAN_AT_SERVICE))
		pi = service_props;
	else
		g_set_error(error, NB_ERROR, NB_ERROR_ILLEGAL_ARGUMENT,
				"illegal %s argument", access_type);
	return pi;
}

gboolean connman_property_dbus2json(const char *access_type,
		json_object *jprop, const gchar *key, GVariant *var,
		gboolean *is_config,
		GError **error)
{
	const struct property_info *pi;
	gboolean ret;

	*is_config = FALSE;
	pi = connman_get_property_info(access_type, error);
	if (!pi)
		return FALSE;

	ret = root_property_dbus2json(jprop, pi, key, var, is_config);
	if (!ret)
		g_set_error(error, NB_ERROR, NB_ERROR_UNKNOWN_PROPERTY,
				"unknown %s property %s",
				access_type, key);

	return ret;
}


void connman_decode_call_error(struct network_state *ns,
		const char *access_type, const char *type_arg,
		const char *method,
		GError **error)
{
	if (!error || !*error)
		return;

	if (strstr((*error)->message,
		   "org.freedesktop.DBus.Error.UnknownObject")) {

		if (!strcmp(method, "SetProperty") ||
		    !strcmp(method, "GetProperty") ||
		    !strcmp(method, "ClearProperty")) {

			g_clear_error(error);
			g_set_error(error, NB_ERROR,
					NB_ERROR_UNKNOWN_PROPERTY,
					"unknown %s property on %s",
					access_type, type_arg);

		} else if (!strcmp(method, "Connect") ||
			   !strcmp(method, "Disconnect") ||
			   !strcmp(method, "Remove") ||
			   !strcmp(method, "ResetCounters") ||
			   !strcmp(method, "MoveAfter") ||
			   !strcmp(method, "MoveBefore")) {

			g_clear_error(error);
			g_set_error(error, NB_ERROR,
					NB_ERROR_UNKNOWN_SERVICE,
					"unknown service %s",
					type_arg);

		} else if (!strcmp(method, "Scan")) {

			g_clear_error(error);
			g_set_error(error, NB_ERROR,
					NB_ERROR_UNKNOWN_TECHNOLOGY,
					"unknown technology %s",
					type_arg);
		}
	}
}

GVariant *connman_call(struct network_state *ns,
		const char *access_type, const char *type_arg,
		const char *method, GVariant *params, GError **error)
{
	const char *path;
	const char *interface;
	GVariant *reply;

	if (!type_arg && (!strcmp(access_type, CONNMAN_AT_TECHNOLOGY) ||
			  !strcmp(access_type, CONNMAN_AT_SERVICE))) {
		g_set_error(error, NB_ERROR, NB_ERROR_MISSING_ARGUMENT,
				"missing %s argument",
				access_type);
		return NULL;
	}

	if (!strcmp(access_type, CONNMAN_AT_MANAGER)) {
		path = CONNMAN_MANAGER_PATH;
		interface = CONNMAN_MANAGER_INTERFACE;
	} else if (!strcmp(access_type, CONNMAN_AT_TECHNOLOGY)) {
		path = CONNMAN_TECHNOLOGY_PATH(type_arg);
		interface = CONNMAN_TECHNOLOGY_INTERFACE;
	} else if (!strcmp(access_type, CONNMAN_AT_SERVICE)) {
		path = CONNMAN_SERVICE_PATH(type_arg);
		interface = CONNMAN_SERVICE_INTERFACE;
	} else {
		g_set_error(error, NB_ERROR, NB_ERROR_ILLEGAL_ARGUMENT,
				"illegal %s argument",
				access_type);
		return NULL;
	}

	reply = g_dbus_connection_call_sync(ns->conn,
			CONNMAN_SERVICE, path, interface, method, params,
			NULL, G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT,
			NULL, error);
	connman_decode_call_error(ns, access_type, type_arg, method,
			error);
	if (!reply) {
		if (error && *error)
			g_dbus_error_strip_remote_error(*error);
		AFB_ERROR("Error calling %s%s%s %s method %s",
				access_type,
				type_arg ? "/" : "",
				type_arg ? type_arg : "",
				method,
				error && *error ? (*error)->message :
					"unspecified");
	}

	return reply;
}

static void connman_call_async_ready(GObject *source_object,
		GAsyncResult *res, gpointer user_data)
{
	struct connman_pending_work *cpw = user_data;
	struct network_state *ns = cpw->ns;
	GVariant *result;
	GError *error = NULL;

	result = g_dbus_connection_call_finish(ns->conn, res, &error);

	cpw->callback(cpw->user_data, result, &error);

	g_clear_error(&error);
	g_cancellable_reset(cpw->cancel);
	g_free(cpw);
}

void connman_cancel_call(struct network_state *ns,
		struct connman_pending_work *cpw)
{
	g_cancellable_cancel(cpw->cancel);
}

struct connman_pending_work *
connman_call_async(struct network_state *ns,
		const char *access_type, const char *type_arg,
		const char *method, GVariant *params, GError **error,
		void (*callback)(void *user_data, GVariant *result, GError **error),
		void *user_data)
{
	const char *path;
	const char *interface;
	struct connman_pending_work *cpw;

	if (!type_arg && (!strcmp(access_type, CONNMAN_AT_TECHNOLOGY) ||
			  !strcmp(access_type, CONNMAN_AT_SERVICE))) {
		g_set_error(error, NB_ERROR, NB_ERROR_MISSING_ARGUMENT,
				"missing %s argument",
				access_type);
		return NULL;
	}

	if (!strcmp(access_type, CONNMAN_AT_MANAGER)) {
		path = CONNMAN_MANAGER_PATH;
		interface = CONNMAN_MANAGER_INTERFACE;
	} else if (!strcmp(access_type, CONNMAN_AT_TECHNOLOGY)) {
		path = CONNMAN_TECHNOLOGY_PATH(type_arg);
		interface = CONNMAN_TECHNOLOGY_INTERFACE;
	} else if (!strcmp(access_type, CONNMAN_AT_SERVICE)) {
		path = CONNMAN_SERVICE_PATH(type_arg);
		interface = CONNMAN_SERVICE_INTERFACE;
	} else {
		g_set_error(error, NB_ERROR, NB_ERROR_ILLEGAL_ARGUMENT,
				"illegal %s argument",
				access_type);
		return NULL;
	}

	cpw = g_malloc(sizeof(*cpw));
	if (!cpw) {
		g_set_error(error, NB_ERROR, NB_ERROR_OUT_OF_MEMORY,
				"out of memory");
		return NULL;
	}
	cpw->ns = ns;
	cpw->user_data = user_data;
	cpw->cancel = g_cancellable_new();
	if (!cpw->cancel) {
		g_free(cpw);
		g_set_error(error, NB_ERROR, NB_ERROR_OUT_OF_MEMORY,
				"out of memory");
		return NULL;
	}
	cpw->callback = callback;

	g_dbus_connection_call(ns->conn,
			CONNMAN_SERVICE, path, interface, method, params,
			NULL,	/* reply type */
			G_DBUS_CALL_FLAGS_NONE, DBUS_REPLY_TIMEOUT,
			cpw->cancel,	/* cancellable? */
			connman_call_async_ready,
			cpw);

	return cpw;
}

json_object *connman_get_properties(struct network_state *ns,
		const char *access_type, const char *type_arg,
		GError **error)
{
	const struct property_info *pi = NULL;
	const char *method = NULL;
	GVariant *reply = NULL;
	GVariantIter *array, *array2;
	GVariant *var = NULL;
	const gchar *path = NULL;
	const gchar *key = NULL;
	const gchar *basename;
	json_object *jarray = NULL, *jprop = NULL, *jresp = NULL, *jtype = NULL;
	gboolean is_config;

	pi = connman_get_property_info(access_type, error);
	if (!pi)
		return NULL;

	method = NULL;
	if (!strcmp(access_type, CONNMAN_AT_MANAGER))
		method = "GetProperties";
	else if (!strcmp(access_type, CONNMAN_AT_TECHNOLOGY))
		method = "GetTechnologies";
	else if (!strcmp(access_type, CONNMAN_AT_SERVICE))
		method = "GetServices";

	if (!method) {
		g_set_error(error, NB_ERROR, NB_ERROR_ILLEGAL_ARGUMENT,
				"illegal %s argument",
				access_type);
		return NULL;
	}

	reply = connman_call(ns, CONNMAN_AT_MANAGER, NULL,
			method, NULL, error);
	if (!reply)
		return NULL;

	if (!strcmp(access_type, CONNMAN_AT_MANAGER)) {
		jprop = json_object_new_object();
		g_variant_get(reply, "(a{sv})", &array);
		while (g_variant_iter_loop(array, "{sv}", &key, &var)) {
			root_property_dbus2json(jprop, pi,
					key, var, &is_config);
		}
		g_variant_iter_free(array);
		g_variant_unref(reply);
		jresp = jprop;
	} else {
		if (!type_arg) {
			jarray = json_object_new_array();
			jresp = json_object_new_object();
			json_object_object_add(jresp, "values", jarray);
		}

		g_variant_get(reply, "(a(oa{sv}))", &array);
		while (g_variant_iter_loop(array, "(oa{sv})", &path, &array2)) {

			/* a basename must exist and be at least 1 character wide */
			basename = strrchr(path, '/');
			if (!basename || strlen(basename) <= 1)
				continue;
			basename++;

			if (!type_arg) {
				jtype = json_object_new_object();
				json_object_object_add(jtype, access_type,
						json_object_new_string(basename));
			} else if (g_strcmp0(basename, type_arg))
				continue;

			jprop = json_object_new_object();
			while (g_variant_iter_loop(array2, "{sv}", &key, &var)) {
				root_property_dbus2json(jprop, pi,
						key, var, &is_config);
			}

			if (!type_arg) {
				json_object_object_add(jtype, "properties", jprop);
				json_object_array_add(jarray, jtype);
			}
		}
		g_variant_iter_free(array);
		g_variant_unref(reply);

		if (type_arg && jprop)
			jresp = jprop;

	}

	if (!jresp) {
		if (type_arg)
			g_set_error(error, NB_ERROR, NB_ERROR_ILLEGAL_ARGUMENT,
					"Bad %s %s", access_type, type_arg);
		else
			g_set_error(error, NB_ERROR, NB_ERROR_ILLEGAL_ARGUMENT,
					"No %s", access_type);
	}

	return jresp;
}

json_object *connman_get_property(struct network_state *ns,
		const char *access_type, const char *type_arg,
		gboolean is_json_name, const char *name, GError **error)
{
	const struct property_info *pi;
	json_object *jprop, *jval;

	pi = connman_get_property_info(access_type, error);
	if (!pi)
		return NULL;

	jprop = connman_get_properties(ns, access_type, type_arg, error);
	if (!jprop)
		return NULL;

	jval = get_named_property(pi, is_json_name, name, jprop);
	json_object_put(jprop);

	if (!jval)
		g_set_error(error, NB_ERROR, NB_ERROR_BAD_PROPERTY,
				"Bad property %s on %s%s%s", name,
				access_type,
				type_arg ? "/" : "",
				type_arg ? type_arg : "");
	return jval;
}

/* NOTE: jval is consumed */
gboolean connman_set_property(struct network_state *ns,
		const char *access_type, const char *type_arg,
		gboolean is_json_name, const char *name, json_object *jval,
		GError **error)
{
	const struct property_info *pi;
	GVariant *reply, *arg;
	gboolean is_config;
	gchar *propname;

	/* get start of properties */
	pi = connman_get_property_info(access_type, error);
	if (!pi)
		return FALSE;

	/* get actual property */
	pi = property_by_name(pi, is_json_name, name, &is_config);
	if (!pi) {
		g_set_error(error, NB_ERROR, NB_ERROR_UNKNOWN_PROPERTY,
				"unknown property with name %s", name);
		json_object_put(jval);
		return FALSE;
	}

	/* convert to gvariant */
	arg = property_json_to_gvariant(pi, NULL, NULL, jval, error);

	/* we don't need this anymore */
	json_object_put(jval);
	jval = NULL;

	/* no variant? error */
	if (!arg)
		return FALSE;

	if (!is_config)
		propname = g_strdup(pi->name);
	else
		propname = configuration_dbus_name(pi->name);

	reply = connman_call(ns, access_type, type_arg,
			"SetProperty",
			g_variant_new("(sv)", propname, arg),
			error);

	g_free(propname);

	if (!reply)
		return FALSE;

	g_variant_unref(reply);

	return TRUE;
}
