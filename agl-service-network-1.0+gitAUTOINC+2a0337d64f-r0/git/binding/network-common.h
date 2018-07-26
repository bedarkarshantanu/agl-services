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

#ifndef NETWORK_COMMON_H
#define NETWORK_COMMON_H

#include <stddef.h>

#define _GNU_SOURCE
#include <glib.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <glib-object.h>

#include <json-c/json.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

struct call_work;

struct network_state {
	GMainLoop *loop;
	GDBusConnection *conn;
	guint manager_sub;
	guint technology_sub;
	guint service_sub;
	struct afb_event global_state_event;
	struct afb_event technologies_event;
	struct afb_event technology_properties_event;
	struct afb_event services_event;
	struct afb_event service_properties_event;
	struct afb_event counter_event;
	struct afb_event agent_event;
	int ping_counter;

	/* NOTE: single connection allowed for now */
	/* NOTE: needs locking and a list */
	GMutex cw_mutex;
	int next_cw_id;
	GSList *cw_pending;
	struct call_work *cw;

	/* agent */
	GDBusNodeInfo *introspection_data;
	guint agent_id;
	guint registration_id;
	gchar *agent_path;
	gboolean agent_registered;
};

struct call_work {
	struct network_state *ns;
	int id;
	gchar *access_type;
	gchar *type_arg;
	gchar *method;
	gchar *connman_method;
	struct connman_pending_work *cpw;
	struct afb_req request;
	gchar *agent_method;
	GDBusMethodInvocation *invocation;
};

/* in network-api.c */

/*
 * Unfortunately we can't completely avoid a global here.
 * appfw API does not pass around a user pointer to do so.
 */
extern struct network_state *global_ns;

/* utility methods in network-util.c */

extern gboolean auto_lowercase_keys;

int str2boolean(const char *value);

json_object *json_object_copy(json_object *jval);

gchar *key_dbus_to_json(const gchar *key, gboolean auto_lower);

json_object *simple_gvariant_to_json(GVariant *var, json_object *parent,
		gboolean recurse);

/**
 * Structure for converting from dbus properties to json
 * and vice-versa.
 * Note this is _not_ a generic dbus json bridge since
 * some constructs are not easily mapped.
 */
struct property_info {
	const char *name;	/* the connman property name */
	const char *json_name;	/* the json name (if NULL autoconvert) */
	const char *fmt;
	unsigned int flags;
	const struct property_info *sub;
};

#define PI_CONFIG	(1U << 0)

const struct property_info *property_by_dbus_name(
		const struct property_info *pi,
		const gchar *dbus_name,
		gboolean *is_config);
const struct property_info *property_by_json_name(
		const struct property_info *pi,
		const gchar *json_name,
		gboolean *is_config);
const struct property_info *property_by_name(
		const struct property_info *pi,
		gboolean is_json_name, const gchar *name,
		gboolean *is_config);

gchar *property_get_json_name(const struct property_info *pi,
		const gchar *name);
gchar *property_get_name(const struct property_info *pi,
		const gchar *json_name);

gchar *configuration_dbus_name(const gchar *dbus_name);
gchar *configuration_json_name(const gchar *json_name);

gchar *property_name_dbus2json(const struct property_info *pi,
		gboolean is_config);

json_object *property_dbus2json(
		const struct property_info **pip,
		const gchar *key, GVariant *var,
		gboolean *is_config);

gboolean root_property_dbus2json(
		json_object *jparent,
		const struct property_info *pi,
		const gchar *key, GVariant *var,
		gboolean *is_config);

GVariant *property_json_to_gvariant(
		const struct property_info *pi,
		const char *fmt,	/* if NULL use pi->fmt */
		const struct property_info *pi_parent,
		json_object *jval,
		GError **error);

typedef enum {
	NB_ERROR_BAD_TECHNOLOGY,
	NB_ERROR_BAD_SERVICE,
	NB_ERROR_OUT_OF_MEMORY,
	NB_ERROR_NO_TECHNOLOGIES,
	NB_ERROR_NO_SERVICES,
	NB_ERROR_BAD_PROPERTY,
	NB_ERROR_UNIMPLEMENTED,
	NB_ERROR_UNKNOWN_PROPERTY,
	NB_ERROR_UNKNOWN_TECHNOLOGY,
	NB_ERROR_UNKNOWN_SERVICE,
	NB_ERROR_MISSING_ARGUMENT,
	NB_ERROR_ILLEGAL_ARGUMENT,
	NB_ERROR_CALL_IN_PROGRESS,
} NBError;

#define NB_ERROR (nb_error_quark())

extern GQuark nb_error_quark(void);

json_object *get_property_collect(json_object *jreqprop, json_object *jprop,
		GError **error);
json_object *get_named_property(const struct property_info *pi,
		gboolean is_json_name, const char *name, json_object *jprop);

#endif
