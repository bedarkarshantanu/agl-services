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

struct network_state *global_ns;

/**
 * The global thread
 */
static GThread *global_thread;

struct init_data {
	GCond cond;
	GMutex mutex;
	gboolean init_done;
	struct network_state *ns;	/* before setting global_ns */
	int rc;
};

static void signal_init_done(struct init_data *id, int rc);

static void call_work_lock(struct network_state *ns)
{
	g_mutex_lock(&ns->cw_mutex);
}

static void call_work_unlock(struct network_state *ns)
{
	g_mutex_unlock(&ns->cw_mutex);
}

struct call_work *call_work_lookup_unlocked(
		struct network_state *ns,
		const char *access_type, const char *type_arg,
		const char *method)
{
	struct call_work *cw;
	GSList *list;

	/* we can only allow a single pending call */
	for (list = ns->cw_pending; list; list = g_slist_next(list)) {
		cw = list->data;
		if (!g_strcmp0(access_type, cw->access_type) &&
		    !g_strcmp0(type_arg, cw->type_arg) &&
		    !g_strcmp0(method, cw->method))
			return cw;
	}
	return NULL;
}

struct call_work *call_work_lookup(
		struct network_state *ns,
		const char *access_type, const char *type_arg,
		const char *method)
{
	struct call_work *cw;

	g_mutex_lock(&ns->cw_mutex);
	cw = call_work_lookup_unlocked(ns, access_type, type_arg, method);
	g_mutex_unlock(&ns->cw_mutex);

	return cw;
}

int call_work_pending_id(
		struct network_state *ns,
		const char *access_type, const char *type_arg,
		const char *method)
{
	struct call_work *cw;
	int id = -1;

	g_mutex_lock(&ns->cw_mutex);
	cw = call_work_lookup_unlocked(ns, access_type, type_arg, method);
	if (cw)
		id = cw->id;
	g_mutex_unlock(&ns->cw_mutex);

	return id;
}

struct call_work *call_work_lookup_by_id_unlocked(
		struct network_state *ns, int id)
{
	struct call_work *cw;
	GSList *list;

	/* we can only allow a single pending call */
	for (list = ns->cw_pending; list; list = g_slist_next(list)) {
		cw = list->data;
		if (cw->id == id)
			return cw;
	}
	return NULL;
}

struct call_work *call_work_lookup_by_id(
		struct network_state *ns, int id)
{
	struct call_work *cw;

	g_mutex_lock(&ns->cw_mutex);
	cw = call_work_lookup_by_id_unlocked(ns, id);
	g_mutex_unlock(&ns->cw_mutex);

	return cw;
}

struct call_work *call_work_create_unlocked(struct network_state *ns,
		const char *access_type, const char *type_arg,
		const char *method, const char *connman_method,
		GError **error)
{

	struct call_work *cw = NULL;

	cw = call_work_lookup_unlocked(ns, access_type, type_arg, method);
	if (cw) {
		g_set_error(error, NB_ERROR, NB_ERROR_CALL_IN_PROGRESS,
				"another call in progress (%s/%s/%s)",
				access_type, type_arg, method);
		return NULL;
	}

	/* no other pending; allocate */
	cw = g_malloc0(sizeof(*cw));
	cw->ns = ns;
	do {
		cw->id = ns->next_cw_id;
		if (++ns->next_cw_id < 0)
			ns->next_cw_id = 1;
	} while (call_work_lookup_by_id_unlocked(ns, cw->id));

	cw->access_type = g_strdup(access_type);
	cw->type_arg = g_strdup(type_arg);
	cw->method = g_strdup(method);
	cw->connman_method = g_strdup(connman_method);

	ns->cw_pending = g_slist_prepend(ns->cw_pending, cw);

	return cw;
}

struct call_work *call_work_create(struct network_state *ns,
		const char *access_type, const char *type_arg,
		const char *method, const char *connman_method,
		GError **error)
{

	struct call_work *cw;

	g_mutex_lock(&ns->cw_mutex);
	cw = call_work_create_unlocked(ns,
			access_type, type_arg, method, connman_method,
			error);
	g_mutex_unlock(&ns->cw_mutex);

	return cw;
}

void call_work_destroy_unlocked(struct call_work *cw)
{
	struct network_state *ns = cw->ns;
	struct call_work *cw2;

	/* verify that it's something we know about */
	cw2 = call_work_lookup_by_id_unlocked(ns, cw->id);
	if (cw2 != cw) {
		AFB_ERROR("Bad call work to destroy");
		return;
	}

	/* remove it */
	ns->cw_pending = g_slist_remove(ns->cw_pending, cw);

	g_free(cw->access_type);
	g_free(cw->type_arg);
	g_free(cw->method);
	g_free(cw->connman_method);
}

void call_work_destroy(struct call_work *cw)
{
	struct network_state *ns = cw->ns;

	g_mutex_lock(&ns->cw_mutex);
	call_work_destroy_unlocked(cw);
	g_mutex_unlock(&ns->cw_mutex);
}

static struct afb_event *get_event_from_value(struct network_state *ns,
			const char *value)
{
	if (!g_strcmp0(value, "global_state"))
		return &ns->global_state_event;

	if (!g_strcmp0(value, "technologies"))
		return &ns->technologies_event;

	if (!g_strcmp0(value, "technology_properties"))
		return &ns->technology_properties_event;

	if (!g_strcmp0(value, "services"))
		return &ns->services_event;

	if (!g_strcmp0(value, "service_properties"))
		return &ns->service_properties_event;

	if (!g_strcmp0(value, "counter"))
		return &ns->counter_event;

	if (!g_strcmp0(value, "agent"))
		return &ns->agent_event;

	return NULL;
}

static void network_manager_signal_callback(
	GDBusConnection *connection,
	const gchar *sender_name,
	const gchar *object_path,
	const gchar *interface_name,
	const gchar *signal_name,
	GVariant *parameters,
	gpointer user_data)
{
	struct network_state *ns = user_data;
	GVariantIter *array1, *array2, *array3;
	GError *error = NULL;
	GVariant *var = NULL;
	const gchar *path = NULL;
	const gchar *key = NULL;
	const gchar *basename;
	json_object *jarray = NULL, *jresp = NULL, *jobj, *jprop;
	struct afb_event *event = NULL;
	GVariantIter *array;
	gboolean is_config, ret;

	/* AFB_INFO("sender=%s", sender_name);
	AFB_INFO("object_path=%s", object_path);
	AFB_INFO("interface=%s", interface_name);
	AFB_INFO("signal=%s", signal_name); */

	if (!g_strcmp0(signal_name, "TechnologyAdded")) {

		g_variant_get(parameters, "(oa{sv})", &path, &array);

		basename = connman_strip_path(path);
		g_assert(basename);	/* guaranteed by dbus */

		jresp = json_object_new_object();

		json_object_object_add(jresp, "technology",
				json_object_new_string(basename));
		json_object_object_add(jresp, "action",
				json_object_new_string("added"));

		jobj = json_object_new_object();
		while (g_variant_iter_loop(array, "{sv}", &key, &var)) {
			ret = technology_property_dbus2json(jobj,
					key, var, &is_config, &error);
			if (!ret) {
				AFB_WARNING("%s property %s - %s",
						"technology",
						key, error->message);
				g_clear_error(&error);
			}
		}
		g_variant_iter_free(array);

		json_object_object_add(jresp, "properties", jobj);

		event = &ns->technologies_event;


	} else if (!g_strcmp0(signal_name, "TechnologyRemoved")) {

		g_variant_get(parameters, "(o)", &path);

		basename = connman_strip_path(path);
		g_assert(basename);	/* guaranteed by dbus */

		jresp = json_object_new_object();

		json_object_object_add(jresp, "technology",
				json_object_new_string(basename));
		json_object_object_add(jresp, "action",
				json_object_new_string("removed"));

		event = &ns->technologies_event;

	} else if (!g_strcmp0(signal_name, "ServicesChanged")) {

		jresp = json_object_new_object();
		jarray = json_object_new_array();
		json_object_object_add(jresp, "values", jarray);

		g_variant_get(parameters, "(a(oa{sv})ao)", &array1, &array2);

		while (g_variant_iter_loop(array1, "(oa{sv})", &path, &array3)) {

			basename = connman_strip_path(path);
			g_assert(basename);	/* guaranteed by dbus */

			jobj = json_object_new_object();

			json_object_object_add(jobj, "service",
					json_object_new_string(basename));

			jprop = NULL;
			while (g_variant_iter_loop(array3,
						"{sv}", &key, &var)) {
				if (!jprop)
					jprop = json_object_new_object();
				ret = service_property_dbus2json(jprop, key, var,
						&is_config, &error);
				if (!ret) {
					AFB_WARNING("%s property %s - %s",
							"service",
							key, error->message);
					g_clear_error(&error);
				}
			}

			json_object_object_add(jobj, "action",
					json_object_new_string(jprop ?
						"changed" : "unchanged"));

			if (jprop)
				json_object_object_add(jobj, "properties",
						jprop);

			json_object_array_add(jarray, jobj);
		}

		while (g_variant_iter_loop(array2, "o", &path)) {

			basename = connman_strip_path(path);
			g_assert(basename);	/* guaranteed by dbus */

			jobj = json_object_new_object();

			json_object_object_add(jobj, "service",
					json_object_new_string(basename));

			json_object_object_add(jobj, "action",
					json_object_new_string("removed"));

			json_object_array_add(jarray, jobj);
		}

		g_variant_iter_free(array2);
		g_variant_iter_free(array1);

		event = &ns->services_event;

	} else if (!g_strcmp0(signal_name, "PropertyChanged")) {

		jresp = json_object_new_object();

		g_variant_get(parameters, "(sv)", &key, &var);
		g_clear_error(&error);
		ret = manager_property_dbus2json(jresp,
				key, var, &is_config, &error);
		if (!ret) {
			AFB_WARNING("%s property %s - %s",
					"manager",
					key, error->message);
			g_clear_error(&error);
			json_object_put(jresp);
			jresp = NULL;
		} else
			event = &ns->global_state_event;
	}

	/* if (jresp)
		printf("manager-signal\n%s\n",
			json_object_to_json_string_ext(jresp,
					JSON_C_TO_STRING_PRETTY)); */

	if (event && jresp) {
		afb_event_push(*event, jresp);
		jresp = NULL;
	}

	json_object_put(jresp);
}

static void network_technology_signal_callback(
	GDBusConnection *connection,
	const gchar *sender_name,
	const gchar *object_path,
	const gchar *interface_name,
	const gchar *signal_name,
	GVariant *parameters,
	gpointer user_data)
{
	struct network_state *ns = user_data;
	GError *error = NULL;
	GVariant *var = NULL;
	const gchar *key = NULL;
	const gchar *basename;
	json_object *jresp = NULL, *jobj;
	struct afb_event *event = NULL;
	gboolean is_config, ret;

	/* AFB_INFO("sender=%s", sender_name);
	AFB_INFO("object_path=%s", object_path);
	AFB_INFO("interface=%s", interface_name);
	AFB_INFO("signal=%s", signal_name); */

	/* a basename must exist and be at least 1 character wide */
	basename = connman_strip_path(object_path);
	g_assert(basename);

	if (!g_strcmp0(signal_name, "PropertyChanged")) {

		jobj = json_object_new_object();

		g_variant_get(parameters, "(sv)", &key, &var);

		g_clear_error(&error);
		ret = technology_property_dbus2json(jobj, key, var, &is_config,
					&error) ;
		g_variant_unref(var);
		var = NULL;

		if (!ret) {
			AFB_ERROR("unhandled manager property %s - %s",
				key, error->message);
			json_object_put(jobj);
			return;
		}

		jresp = json_object_new_object();

		json_object_object_add(jresp, "technology",
				json_object_new_string(basename));

		json_object_object_add(jresp, "properties", jobj);
		event = &ns->technology_properties_event;
	}

	/* if (jresp)
		printf("technology-signal\n%s\n",
				json_object_to_json_string_ext(jresp,
					JSON_C_TO_STRING_PRETTY)); */

	if (event && jresp) {
		afb_event_push(*event, jresp);
		jresp = NULL;
	}

	json_object_put(jresp);
}

static void network_service_signal_callback(
	GDBusConnection *connection,
	const gchar *sender_name,
	const gchar *object_path,
	const gchar *interface_name,
	const gchar *signal_name,
	GVariant *parameters,
	gpointer user_data)
{
	struct network_state *ns = user_data;
	GError *error = NULL;
	GVariant *var = NULL;
	const gchar *key = NULL;
	const gchar *basename;
	json_object *jresp = NULL, *jobj;
	struct afb_event *event = NULL;
	gboolean is_config, ret;

	/* AFB_INFO("sender=%s", sender_name);
	AFB_INFO("object_path=%s", object_path);
	AFB_INFO("interface=%s", interface_name);
	AFB_INFO("signal=%s", signal_name); */

	/* a basename must exist and be at least 1 character wide */
	basename = connman_strip_path(object_path);
	g_assert(basename);

	if (!g_strcmp0(signal_name, "PropertyChanged")) {

		g_variant_get(parameters, "(sv)", &key, &var);

		jobj = json_object_new_object();
		ret = service_property_dbus2json(jobj,
				key, var, &is_config, &error);

		g_variant_unref(var);
		var = NULL;

		if (!ret) {
			AFB_ERROR("unhandled %s property %s - %s",
				"service", key, error->message);
			json_object_put(jobj);
			return;
		}

		jresp = json_object_new_object();

		json_object_object_add(jresp, "service",
				json_object_new_string(basename));

		json_object_object_add(jresp, "properties",
				jobj);

		event = &ns->service_properties_event;
	}

	/* if (jresp)
		printf("service-signal\n%s\n",
				json_object_to_json_string_ext(jresp,
					JSON_C_TO_STRING_PRETTY)); */

	if (event && jresp) {
		afb_event_push(*event, jresp);
		jresp = NULL;
	}

	json_object_put(jresp);
}

/* Introspection data for the agent service */
static const gchar introspection_xml[] =
"<node>"
"  <interface name='net.connman.Agent'>"
"    <method name='RequestInput'>"
"	   <arg type='o' name='service' direction='in'/>"
"	   <arg type='a{sv}' name='fields' direction='in'/>"
"	   <arg type='a{sv}' name='fields' direction='out'/>"
"    </method>"
"    <method name='ReportError'>"
"	   <arg type='o' name='service' direction='in'/>"
"	   <arg type='s' name='error' direction='in'/>"
"    </method>"
"  </interface>"
"</node>";

static const struct property_info agent_request_input_props[] = {
	{
		.name = "Name",
		.fmt = "{sv}",
		.sub	= (const struct property_info []) {
			{ .name = "Type",		.fmt = "s", },
			{ .name = "Requirement",	.fmt = "s", },
			{ .name = "Alternates",		.fmt = "s", },
			{ },
		},
	}, {
		.name = "SSID",
		.fmt = "{sv}",
		.sub	= (const struct property_info []) {
			{ .name = "Type",		.fmt = "s", },
			{ .name = "Requirement",	.fmt = "s", },
			{ },
		},
	}, {
		.name = "Identity",
		.fmt = "{sv}",
		.sub	= (const struct property_info []) {
			{ .name = "Type",		.fmt = "s", },
			{ .name = "Requirement",	.fmt = "s", },
			{ },
		},
	}, {
		.name = "Passphrase",
		.fmt = "{sv}",
		.sub	= (const struct property_info []) {
			{ .name = "Type",		.fmt = "s", },
			{ .name = "Requirement",	.fmt = "s", },
			{ },
		},
	}, {
		.name = "WPS",
		.fmt = "{sv}",
		.sub	= (const struct property_info []) {
			{ .name = "Type",		.fmt = "s", },
			{ .name = "Requirement",	.fmt = "s", },
			{ },
		},
	},
	{ }
};

static const struct property_info agent_request_input_out_props[] = {
	{
		.name = "Name",
		.fmt = "s",
	}, {
		.name = "SSID",
		.fmt = "s",
	}, {
		.name = "Identity",
		.fmt = "s",
	}, {
		.name = "Passphrase",
		.fmt = "s",
	}, {
		.name = "WPS",
		.fmt = "s",
	},
	{ }
};

static void handle_method_call(
		GDBusConnection *connection,
		const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *method_name,
		GVariant *parameters,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct network_state *ns = user_data;
	struct call_work *cw;
	json_object *jev = NULL, *jprop;
	GVariantIter *array;
	const gchar *path = NULL;
	const gchar *service = NULL;
	const gchar *key = NULL;
	const gchar *strerr = NULL;
	GVariant *var = NULL;
	gboolean is_config;

	/* AFB_INFO("sender=%s", sender_name);
	AFB_INFO("object_path=%s", object_path);
	AFB_INFO("interface=%s", interface_name);
	AFB_INFO("method=%s", method_name); */

	if (!g_strcmp0(method_name, "RequestInput")) {

		jev = json_object_new_object();
		jprop = json_object_new_object();
		g_variant_get(parameters, "(oa{sv})", &path, &array);
		while (g_variant_iter_loop(array, "{sv}", &key, &var)) {
			root_property_dbus2json(jprop, agent_request_input_props,
					key, var, &is_config);
		}
		g_variant_iter_free(array);

		service = connman_strip_path(path);

		call_work_lock(ns);

		/* can only occur while a connect is issued */
		cw = call_work_lookup_unlocked(ns, "service", service,
				"connect_service");

		/* if nothing is pending return an error */
		if (!cw) {
			call_work_unlock(ns);
			json_object_put(jprop);
			g_dbus_method_invocation_return_dbus_error(invocation,
					"net.connman.Agent.Error.Canceled",
					"No connection pending");
			return;
		}

		json_object_object_add(jev, "id",
			json_object_new_int(cw->id));
		json_object_object_add(jev, "method",
				json_object_new_string("request-input"));
		json_object_object_add(jev, "service",
				json_object_new_string(service));
		json_object_object_add(jev, "fields", jprop);

		cw->agent_method = "RequestInput";
		cw->invocation = invocation;

		call_work_unlock(ns);

		/* AFB_INFO("request-input: jev=%s",
				json_object_to_json_string(jev)); */

		afb_event_push(ns->agent_event, jev);

		return;
	}

	if (!g_strcmp0(method_name, "ReportError")) {

		g_variant_get(parameters, "(os)", &path, &strerr);

		AFB_INFO("report-error: service_path=%s error=%s",
				path, strerr);

		return g_dbus_method_invocation_return_value(invocation, NULL);
	}

	g_dbus_method_invocation_return_dbus_error(invocation,
			"org.freedesktop.DBus.Error.UnknownMethod",
			"Uknown method");
}

static const GDBusInterfaceVTable interface_vtable = {
	.method_call  = handle_method_call,
	.get_property = NULL,
	.set_property = NULL,
};

static void on_bus_acquired(GDBusConnection *connection,
			    const gchar *name, gpointer user_data)
{
	struct init_data *id = user_data;
	struct network_state *ns = id->ns;
	GVariant *result;
	GError *error = NULL;

	AFB_INFO("agent bus acquired - registering %s", ns->agent_path);

	ns->registration_id = g_dbus_connection_register_object(connection,
			ns->agent_path,
			ns->introspection_data->interfaces[0],
			&interface_vtable,
			ns,	/* user data */
			NULL,	/* user_data_free_func */
			NULL);

	if (!ns->registration_id) {
		AFB_ERROR("failed to register agent to dbus");
		goto err_unable_to_register_bus;

	}

	result = manager_call(ns, "RegisterAgent",
			g_variant_new("(o)", ns->agent_path),
				&error);
	if (!result) {
		AFB_ERROR("failed to register agent to connman");
		goto err_unable_to_register_connman;
	}
	g_variant_unref(result);

	ns->agent_registered = TRUE;

	AFB_INFO("agent registered at %s", ns->agent_path);
	signal_init_done(id, 0);

	return;

err_unable_to_register_connman:
	 g_dbus_connection_unregister_object(ns->conn, ns->registration_id);
	 ns->registration_id = 0;
err_unable_to_register_bus:
	signal_init_done(id, -1);
}

static int network_register_agent(struct init_data *id)
{
	struct network_state *ns = id->ns;

	ns->agent_path = g_strdup_printf("%s/agent%d",
			CONNMAN_PATH, getpid());
	if (!ns->agent_path) {
		AFB_ERROR("can't create agent path");
		goto out_no_agent_path;
	}

	ns->introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
	if (!ns->introspection_data) {
		AFB_ERROR("can't create introspection data");
		goto out_no_introspection_data;
	}

	ns->agent_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, AGENT_SERVICE,
			G_BUS_NAME_OWNER_FLAGS_REPLACE |
			  G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
			on_bus_acquired,
			NULL,
			NULL,
			id,
			NULL);
	if (!ns->agent_id) {
		AFB_ERROR("can't create agent bus instance");
		goto out_no_bus_name;
	}

	return 0;

out_no_bus_name:
	g_dbus_node_info_unref(ns->introspection_data);
out_no_introspection_data:
	g_free(ns->agent_path);
out_no_agent_path:
	return -1;
}

static void network_unregister_agent(struct network_state *ns)
{
	g_bus_unown_name(ns->agent_id);
	g_dbus_node_info_unref(ns->introspection_data);
	g_free(ns->agent_path);
}

static struct network_state *network_init(GMainLoop *loop)
{
	struct network_state *ns;
	GError *error = NULL;

	ns = g_try_malloc0(sizeof(*ns));
	if (!ns) {
		AFB_ERROR("out of memory allocating network state");
		goto err_no_ns;
	}

	AFB_INFO("connecting to dbus");

	ns->loop = loop;
	ns->conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (!ns->conn) {
		if (error)
			g_dbus_error_strip_remote_error(error);
		AFB_ERROR("Cannot connect to D-Bus, %s",
				error ? error->message : "unspecified");
		g_error_free(error);
		goto err_no_conn;

	}

	AFB_INFO("connected to dbus");

	ns->global_state_event =
		afb_daemon_make_event("global_state");
	ns->technologies_event =
		afb_daemon_make_event("technologies");
	ns->technology_properties_event =
		afb_daemon_make_event("technology_properties");
	ns->services_event =
		afb_daemon_make_event("services");
	ns->service_properties_event =
		afb_daemon_make_event("service_properties");
	ns->counter_event =
		afb_daemon_make_event("counter");
	ns->agent_event =
		afb_daemon_make_event("agent");

	if (!afb_event_is_valid(ns->global_state_event) ||
	    !afb_event_is_valid(ns->technologies_event) ||
	    !afb_event_is_valid(ns->technology_properties_event) ||
	    !afb_event_is_valid(ns->services_event) ||
	    !afb_event_is_valid(ns->service_properties_event) ||
	    !afb_event_is_valid(ns->counter_event) ||
	    !afb_event_is_valid(ns->agent_event)) {
		AFB_ERROR("Cannot create events");
		goto err_no_events;
	}

	ns->manager_sub = g_dbus_connection_signal_subscribe(
			ns->conn,
			NULL,	/* sender */
			CONNMAN_MANAGER_INTERFACE,
			NULL,	/* member */
			NULL,	/* object path */
			NULL,	/* arg0 */
			G_DBUS_SIGNAL_FLAGS_NONE,
			network_manager_signal_callback,
			ns,
			NULL);
	if (!ns->manager_sub) {
		AFB_ERROR("Unable to subscribe to manager signal");
		goto err_no_manager_sub;
	}

	ns->technology_sub = g_dbus_connection_signal_subscribe(
			ns->conn,
			NULL,	/* sender */
			CONNMAN_TECHNOLOGY_INTERFACE,
			NULL,	/* member */
			NULL,	/* object path */
			NULL,	/* arg0 */
			G_DBUS_SIGNAL_FLAGS_NONE,
			network_technology_signal_callback,
			ns,
			NULL);
	if (!ns->technology_sub) {
		AFB_ERROR("Unable to subscribe to technology signal");
		goto err_no_technology_sub;
	}

	ns->service_sub = g_dbus_connection_signal_subscribe(
			ns->conn,
			NULL,	/* sender */
			CONNMAN_SERVICE_INTERFACE,
			NULL,	/* member */
			NULL,	/* object path */
			NULL,	/* arg0 */
			G_DBUS_SIGNAL_FLAGS_NONE,
			network_service_signal_callback,
			ns,
			NULL);
	if (!ns->service_sub) {
		AFB_ERROR("Unable to subscribe to service signal");
		goto err_no_service_sub;
	}

	g_mutex_init(&ns->cw_mutex);
	ns->next_cw_id = 1;

	return ns;

err_no_service_sub:
	g_dbus_connection_signal_unsubscribe(ns->conn, ns->technology_sub);
err_no_technology_sub:
	g_dbus_connection_signal_unsubscribe(ns->conn, ns->manager_sub);
err_no_manager_sub:
	/* no way to clear the events */
err_no_events:
	g_dbus_connection_close(ns->conn, NULL, NULL, NULL);
err_no_conn:
	g_free(ns);
err_no_ns:
	return NULL;
}

static void network_cleanup(struct network_state *ns)
{
	g_dbus_connection_signal_unsubscribe(ns->conn, ns->service_sub);
	g_dbus_connection_signal_unsubscribe(ns->conn, ns->technology_sub);
	g_dbus_connection_signal_unsubscribe(ns->conn, ns->manager_sub);
	g_dbus_connection_close(ns->conn, NULL, NULL, NULL);
	g_free(ns);
}

static void signal_init_done(struct init_data *id, int rc)
{
	g_mutex_lock(&id->mutex);
	id->init_done = TRUE;
	id->rc = rc;
	g_cond_signal(&id->cond);
	g_mutex_unlock(&id->mutex);
}

static gpointer network_func(gpointer ptr)
{
	struct init_data *id = ptr;
	struct network_state *ns;
	GMainLoop *loop;
	int rc;

	loop = g_main_loop_new(NULL, FALSE);
	if (!loop) {
		AFB_ERROR("Unable to create main loop");
		goto err_no_loop;
	}

	/* real network init */
	ns = network_init(loop);
	if (!ns) {
		AFB_ERROR("network_init() failed");
		goto err_no_ns;
	}

	id->ns = ns;
	rc = network_register_agent(id);
	if (rc) {
		AFB_ERROR("network_register_agent() failed");
		goto err_no_agent;
	}

	/* note that we wait for agent registration to signal done */

	global_ns = ns;
	g_main_loop_run(loop);

	g_main_loop_unref(ns->loop);

	network_unregister_agent(ns);

	network_cleanup(ns);
	global_ns = NULL;

	return NULL;

err_no_agent:
	network_cleanup(ns);

err_no_ns:
	g_main_loop_unref(loop);

err_no_loop:
	signal_init_done(id, -1);

	return NULL;
}

static int init(void)
{
	struct init_data init_data, *id = &init_data;
	gint64 end_time;

	memset(id, 0, sizeof(*id));
	id->init_done = FALSE;
	id->rc = 0;
	g_cond_init(&id->cond);
	g_mutex_init(&id->mutex);

	global_thread = g_thread_new("agl-service-network",
				network_func,
				id);

	AFB_INFO("network-binding waiting for init done");

	/* wait maximum 10 seconds for init done */
	end_time = g_get_monotonic_time () + 10 * G_TIME_SPAN_SECOND;
	g_mutex_lock(&id->mutex);
	while (!id->init_done) {
		if (!g_cond_wait_until(&id->cond, &id->mutex, end_time))
			break;
	}
	g_mutex_unlock(&id->mutex);

	if (!id->init_done) {
		AFB_ERROR("network-binding init timeout");
		return -1;
	}

	if (id->rc)
		AFB_ERROR("network-binding init thread returned %d",
				id->rc);
	else
		AFB_INFO("network-binding operational");

	return id->rc;
}

static void network_subscribe_unsubscribe(struct afb_req request,
		gboolean unsub)
{
	struct network_state *ns = global_ns;
	json_object *jresp = json_object_new_object();
	const char *value;
	struct afb_event *event;
	int rc;

	/* if value exists means to set offline mode */
	value = afb_req_value(request, "value");
	if (!value) {
		afb_req_fail_f(request, "failed", "Missing \"value\" event");
		return;
	}

	event = get_event_from_value(ns, value);
	if (!event) {
		afb_req_fail_f(request, "failed", "Bad \"value\" event \"%s\"",
				value);
		return;
	}

	if (!unsub)
		rc = afb_req_subscribe(request, *event);
	else
		rc = afb_req_unsubscribe(request, *event);
	if (rc != 0) {
		afb_req_fail_f(request, "failed",
					"%s error on \"value\" event \"%s\"",
					!unsub ? "subscribe" : "unsubscribe",
					value);
		return;
	}

	afb_req_success_f(request, jresp, "Network %s to event \"%s\"",
			!unsub ? "subscribed" : "unsubscribed",
			value);
}

static void network_subscribe(struct afb_req request)
{
	network_subscribe_unsubscribe(request, FALSE);
}

static void network_unsubscribe(struct afb_req request)
{
	network_subscribe_unsubscribe(request, TRUE);
}

static void network_state(struct afb_req request)
{
	struct network_state *ns = global_ns;
	GError *error = NULL;
	json_object *jresp;

	jresp = manager_get_property(ns, FALSE, "State", &error);
	if (!jresp) {
		afb_req_fail_f(request, "failed", "property %s error %s",
				"State", error->message);
		return;
	}

	afb_req_success(request, jresp, "Network - state");
}

static void network_offline(struct afb_req request)
{
	struct network_state *ns = global_ns;
	GError *error = NULL;
	json_object *jresp = NULL;
	const char *value;
	int set_to;
	gboolean ret;

	/* if value exists means to set offline mode */
	value = afb_req_value(request, "value");
	if (!value) {
		jresp = manager_get_property(ns, FALSE, "OfflineMode", &error);
		if (!jresp) {
			afb_req_fail_f(request, "failed", "property %s error %s",
					"OfflineMode", error->message);
			g_error_free(error);
			return;
		}
	} else {
		set_to = str2boolean(value);
		if (set_to < 0) {
			afb_req_fail_f(request, "failed", "bad value \"%s\"",
					value);
			return;
		}
		ret = manager_set_property(ns, FALSE, "OfflineMode",
				json_object_new_boolean(set_to), &error);
		if (!ret) {
			afb_req_fail_f(request, "failed",
				"Network - offline mode set to %s failed - %s",
				set_to ? "true" : "false", error->message);
			g_error_free(error);
			return;
		}
	}

	afb_req_success(request, jresp, "Network - offline mode");
}

static void network_technologies(struct afb_req request)
{
	struct network_state *ns = global_ns;
	json_object *jresp;
	GError *error = NULL;

	/* if value exists means to set offline mode */
	jresp = technology_properties(ns, &error, NULL);
	if (!jresp) {
		afb_req_fail_f(request, "failed", "technology properties error %s",
				error->message);
		g_error_free(error);
		return;
	}

	afb_req_success(request, jresp, "Network - Network Technologies");
}

static void network_services(struct afb_req request)
{
	struct network_state *ns = global_ns;
	json_object *jresp;
	GError *error = NULL;

	jresp = service_properties(ns, &error, NULL);
	if (!jresp) {
		afb_req_fail_f(request, "failed", "service properties error %s",
				error->message);
		g_error_free(error);
		return;
	}

	afb_req_success(request, jresp, "Network - Network Services");
}

static void network_technology_set_powered(struct afb_req request,
		gboolean powered)
{
	struct network_state *ns = global_ns;
	GError *error = NULL;
	const char *technology;
	json_object *jpowered;
	gboolean ret, this_powered;

	/* if value exists means to set offline mode */
	technology = afb_req_value(request, "technology");
	if (!technology) {
		afb_req_fail(request, "failed",
				"technology argument missing");
		return;
	}

	jpowered = technology_get_property(ns, technology,
			FALSE, "Powered", &error);
	if (!jpowered) {
		afb_req_fail_f(request, "failed",
			"Network - failed to get current Powered state - %s",
			error->message);
		g_error_free(error);
		return;
	}
	this_powered = json_object_get_boolean(jpowered);
	json_object_put(jpowered);
	jpowered = NULL;

	if (this_powered == powered) {
		afb_req_success_f(request, NULL,
				"Network - Technology %s already %s",
				technology, powered ? "enabled" : "disabled");
		return;
	}

	ret = technology_set_property(ns, technology,
			FALSE, "Powered",
			json_object_new_boolean(powered), &error);
	if (!ret) {
		afb_req_fail_f(request, "failed",
			"Network - failed to set Powered state - %s",
			error->message);
		g_error_free(error);
		return;
	}

	afb_req_success_f(request, NULL, "Network - Technology %s %s",
			technology, powered ? "enabled" : "disabled");
}

static void network_enable_technology(struct afb_req request)
{
	return network_technology_set_powered(request, TRUE);
}

static void network_disable_technology(struct afb_req request)
{
	return network_technology_set_powered(request, FALSE);
}

static void network_scan_services(struct afb_req request)
{
	struct network_state *ns = global_ns;
	GVariant *reply = NULL;
	GError *error = NULL;
	const char *technology;

	/* if value exists means to set offline mode */
	technology = afb_req_value(request, "technology");
	if (!technology) {
		afb_req_fail(request, "failed", "No technology given to enable");
		return;
	}

	reply = technology_call(ns, technology, "Scan", NULL, &error);
	if (!reply) {
		afb_req_fail_f(request, "failed",
				"technology %s method %s error %s",
				technology, "Scan", error->message);
		g_error_free(error);
		return;
	}
	g_variant_unref(reply);

	afb_req_success_f(request, json_object_new_object(),
			"Network - technology %s scan complete",
			technology);
}

static void network_move_service(struct afb_req request)
{
	struct network_state *ns = global_ns;
	GVariant *reply = NULL;
	GError *error = NULL;
	const char *service;
	const char *other_service;
	const char *method_name;
	gboolean before_after;	/* false=before, true=after */

	/* first service */
	service = afb_req_value(request, "service");
	if (!service) {
		afb_req_fail(request, "failed", "No service given to move");
		return;
	}

	before_after = FALSE;
	other_service = afb_req_value(request, "before_service");
	if (!other_service) {
		before_after = TRUE;
		other_service = afb_req_value(request, "after_service");
	}

	if (!other_service) {
		afb_req_fail(request, "failed", "No other service given for move");
		return;
	}

	method_name = before_after ? "MoveAfter" : "MoveBefore";

	reply = service_call(ns, service, method_name,
			g_variant_new("o", CONNMAN_SERVICE_PATH(other_service)),
			&error);
	if (!reply) {
		afb_req_fail_f(request, "failed", "%s error %s",
				method_name,
				error ? error->message : "unspecified");
		g_error_free(error);
		return;
	}
	g_variant_unref(reply);

	afb_req_success_f(request, NULL, "Network - service %s moved %s service %s",
			service, before_after ? "before" : "after", other_service);
}

static void network_remove_service(struct afb_req request)
{
	struct network_state *ns = global_ns;
	GVariant *reply = NULL;
	GError *error = NULL;
	const char *service;

	/* first service */
	service = afb_req_value(request, "service");
	if (!service) {
		afb_req_fail(request, "failed", "No service");
		return;
	}

	reply = service_call(ns, service, "Remove", NULL, &error);
	if (!reply) {
		afb_req_fail_f(request, "failed", "Remove error %s",
				error ? error->message : "unspecified");
		g_error_free(error);
		return;
	}
	g_variant_unref(reply);

	afb_req_success_f(request, NULL, "Network - service %s removed",
			service);
}

/* reset_counters as async implementation for illustrative purposes */

static void reset_counters_callback(void *user_data,
		GVariant *result, GError **error)
{
	struct call_work *cw = user_data;
	struct network_state *ns = cw->ns;

	connman_decode_call_error(ns,
		cw->access_type, cw->type_arg, cw->connman_method,
		error);
	if (error && *error) {
		afb_req_fail_f(cw->request, "failed", "%s error %s",
				cw->method, (*error)->message);
		goto out_free;
	}

	if (result)
		g_variant_unref(result);

	afb_req_success_f(cw->request, NULL, "Network - service %s %s",
			cw->type_arg, cw->method);
out_free:
	afb_req_unref(cw->request);
	call_work_destroy(cw);
}

static void network_reset_counters(struct afb_req request)
{
	struct network_state *ns = global_ns;
	GError *error = NULL;
	const char *service;
	struct call_work *cw;

	/* first service */
	service = afb_req_value(request, "service");
	if (!service) {
		afb_req_fail(request, "failed",
				"No service given");
		return;
	}

	cw = call_work_create(ns, "service", service,
			"reset_counters", "ResetCounters", &error);
	if (!cw) {
		afb_req_fail_f(request, "failed", "can't queue work %s",
				error->message);
		g_error_free(error);
		return;
	}

	cw->request = request;
	afb_req_addref(request);
	cw->cpw = connman_call_async(ns, "service", service,
			"ResetCounters", NULL, &error,
			reset_counters_callback, cw);
	if (!cw->cpw) {
		afb_req_fail_f(request, "failed", "reset counters error %s",
				error->message);
		call_work_destroy(cw);
		g_error_free(error);
		return;
	}
}

static void connect_service_callback(void *user_data,
		GVariant *result, GError **error)
{
	struct call_work *cw = user_data;
	struct network_state *ns = cw->ns;
	struct json_object *jerr;
	GError *sub_error = NULL;

	connman_decode_call_error(ns,
		cw->access_type, cw->type_arg, cw->connman_method,
		error);
	if (error && *error) {
		/* read the Error property (if available to be specific) */

		jerr = service_get_property(ns, cw->type_arg, FALSE,
				"Error", &sub_error);
		g_clear_error(&sub_error);
		if (jerr) {
			/* clear property error */
			service_call(ns, cw->type_arg, "ClearProperty",
					NULL, &sub_error);
			g_clear_error(&sub_error);

			afb_req_fail_f(cw->request, "failed", "Connect error: %s",
				json_object_get_string(jerr));
			json_object_put(jerr);
			jerr = NULL;
		} else
			afb_req_fail_f(cw->request, "failed", "Connect error: %s",
				(*error)->message);
		goto out_free;
	}

	if (result)
		g_variant_unref(result);

	afb_req_success_f(cw->request, NULL, "Network - service %s connected",
			cw->type_arg);
out_free:
	afb_req_unref(cw->request);
	call_work_destroy(cw);
}

static void network_connect_service(struct afb_req request)
{
	struct network_state *ns = global_ns;
	GError *error = NULL;
	const char *service;
	struct call_work *cw;

	/* first service */
	service = afb_req_value(request, "service");
	if (!service) {
		afb_req_fail(request, "failed",
				"No service given");
		return;
	}

	cw = call_work_create(ns, "service", service,
			"connect_service", "Connect", &error);
	if (!cw) {
		afb_req_fail_f(request, "failed", "can't queue work %s",
				error->message);
		g_error_free(error);
		return;
	}

	cw->request = request;
	afb_req_addref(request);
	cw->cpw = connman_call_async(ns, "service", service,
			"Connect", NULL, &error,
			connect_service_callback, cw);
	if (!cw->cpw) {
		afb_req_fail_f(request, "failed", "connection error %s",
				error->message);
		call_work_destroy(cw);
		g_error_free(error);
		return;
	}
}

static void network_disconnect_service(struct afb_req request)
{
	struct network_state *ns = global_ns;
	json_object *jresp;
	GVariant *reply = NULL;
	GError *error = NULL;
	const char *service;

	/* first service */
	service = afb_req_value(request, "service");
	if (!service) {
		afb_req_fail(request, "failed", "No service given to move");
		return;
	}

	reply = service_call(ns, service, "Disconnect", NULL, &error);
	if (!reply) {
		afb_req_fail_f(request, "failed", "Disconnect error %s",
				error ? error->message : "unspecified");
		g_error_free(error);
		return;
	}

	g_variant_unref(reply);

	jresp = json_object_new_object();
	afb_req_success_f(request, jresp, "Network - service %s disconnected",
			service);
}

static void network_get_property(struct afb_req request)
{
	struct network_state *ns = global_ns;
	json_object *jobj = afb_req_json(request);
	const char *technology, *service;
	const char *access_type;
	const char *type_arg;
	GError *error = NULL;
	json_object *jprop, *jrespprop = NULL, *jreqprop = NULL;

	(void)ns;

	/* printf("%s\n", json_object_to_json_string_ext(jobj,
					JSON_C_TO_STRING_PRETTY)); */

	/* either or both may be NULL */
	technology = afb_req_value(request, "technology");
	service = afb_req_value(request, "service");

	if (technology) {
		access_type = CONNMAN_AT_TECHNOLOGY;
		type_arg = technology;
	} else if (service) {
		access_type = CONNMAN_AT_SERVICE;
		type_arg = service;
	} else {
		access_type = CONNMAN_AT_MANAGER;
		type_arg = NULL;
	}

	jprop = connman_get_properties(ns, access_type, type_arg, &error);
	if (!jprop) {
		afb_req_fail_f(request, "failed", "%s property error %s",
				access_type, error->message);
		g_error_free(error);
		return;
	}

	/* if properties exist, copy out only those */
	if (json_object_object_get_ex(jobj, "properties", &jreqprop)) {
		/* and now copy (if found) */
		jrespprop = get_property_collect(jreqprop, jprop,
				&error);
		json_object_put(jprop);

		if (!jrespprop) {
			afb_req_fail_f(request, "failed", "error - %s",
					error->message);
			g_error_free(error);
			return;
		}
	} else	/* otherwise everything */
		jrespprop = jprop;

	afb_req_success_f(request, jrespprop, "Network - %s property get",
			access_type);
}

static void network_set_property(struct afb_req request)
{
	struct network_state *ns = global_ns;
	json_object *jobj = afb_req_json(request);
	const char *technology, *service;
	const char *access_type;
	const char *type_arg;
	json_object *jprop = NULL;
	GError *error = NULL;
	gboolean ret;
	int count;

	/* printf("%s\n", json_object_to_json_string_ext(jobj,
					JSON_C_TO_STRING_PRETTY)); */

	if (!json_object_object_get_ex(jobj, "properties", &jprop)) {
		afb_req_fail_f(request, "failed", "Network - property set no properties");
		return;
	}

	/* either or both may be NULL */
	technology = afb_req_value(request, "technology");
	service = afb_req_value(request, "service");

	if (technology) {
		access_type = CONNMAN_AT_TECHNOLOGY;
		type_arg = technology;
	} else if (service) {
		access_type = CONNMAN_AT_SERVICE;
		type_arg = service;
	} else {
		access_type = CONNMAN_AT_MANAGER;
		type_arg = NULL;
	}

	/* iterate */
	count = 0;
	json_object_object_foreach(jprop, key, jval) {
		ret = FALSE;

		/* keep jval from being consumed */
		json_object_get(jval);
		ret = connman_set_property(ns, access_type, type_arg,
					TRUE, key, jval, &error);
		if (!ret) {
			afb_req_fail_f(request, "failed",
				"Network - set property %s to %s failed - %s",
				key, json_object_to_json_string(jval),
				error->message);
			g_error_free(error);
			return;
		}
		count++;
	}

	if (!count) {
		afb_req_fail_f(request, "failed", "Network - property set empty");
		return;
	}

	afb_req_success_f(request, NULL, "Network - %s %d propert%s set",
			access_type, count, count > 1 ? "ies" : "y");
}

static void network_agent_response(struct afb_req request)
{
	struct network_state *ns = global_ns;
	json_object *jobj = afb_req_json(request);
	json_object *jfields;
	const char *id_str;
	int id;
	const struct property_info *pi, *pi_sub;
	GError *error = NULL;
	gboolean ret;
	struct call_work *cw;
	GVariant *parameters = NULL, *item;
	GVariantBuilder builder, builder2;
	gboolean is_config;
	gchar *dbus_name;

	/* printf("%s\n", json_object_to_json_string(jobj)); */

	/* either or both may be NULL */
	id_str = afb_req_value(request, "id");
	id = id_str ? (int)strtol(id_str, NULL, 10) : -1;
	if (id <= 0) {
		afb_req_fail_f(request, "failed",
				"Network - agent response missing arguments");
		return;
	}

	call_work_lock(ns);
	cw = call_work_lookup_by_id_unlocked(ns, id);
	if (!cw || !cw->invocation) {
		call_work_unlock(ns);
		afb_req_fail_f(request, "failed",
				"Network - can't find request with id=%d", id);
		return;
	}

	ret = FALSE;
	parameters = NULL;
	if (!g_strcmp0(cw->agent_method, "RequestInput") &&
	    json_object_object_get_ex(jobj, "fields", &jfields)) {

		/* AFB_INFO("request-input response fields=%s",
				json_object_to_json_string(jfields)); */

		pi = agent_request_input_out_props;
		g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
		json_object_object_foreach(jfields, key_o, jval_o) {
			pi_sub = property_by_json_name(pi, key_o, &is_config);
			if (!pi_sub) {
				AFB_ERROR("no property %s", key_o);
				continue;
			}

			g_clear_error(&error);
			item = property_json_to_gvariant(pi_sub, NULL, NULL,
					jval_o, &error);
			if (!item) {
				AFB_ERROR("on %s", key_o);
				continue;
			}

			dbus_name = property_get_name(pi, key_o);
			g_assert(dbus_name);	/* can't fail; but check */

			g_variant_builder_add(&builder, "{sv}", dbus_name, item);

			g_free(dbus_name);
		}

		/* dbus requires response to be wrapped as a tuple */
		g_variant_builder_init(&builder2, G_VARIANT_TYPE_TUPLE);
		g_variant_builder_add_value(&builder2,
				g_variant_builder_end(&builder));

		parameters = g_variant_builder_end(&builder2);
		ret = TRUE;
	}


	if (!ret) {
		afb_req_fail_f(request, "failed",
				"Network - unhandled agent method %s",
				cw->agent_method);
		g_dbus_method_invocation_return_dbus_error(cw->invocation,
				"org.freedesktop.DBus.Error.UnknownMethod",
				"Uknown method");
		cw->invocation = NULL;
		call_work_unlock(ns);
		return;
	}

	g_dbus_method_invocation_return_value(cw->invocation, parameters);
	cw->invocation = NULL;

	call_work_unlock(ns);

	afb_req_success_f(request, NULL, "Network - agent response sent");
}

static const struct afb_verb_v2 network_verbs[] = {
	{
		.verb = "subscribe",
		.session = AFB_SESSION_NONE,
		.callback = network_subscribe,
		.info ="Subscribe to the event of 'value'",
	}, {
		.verb = "unsubscribe",
		.session = AFB_SESSION_NONE,
		.callback = network_unsubscribe,
		.info ="Unsubscribe to the event of 'value'",
	}, {
		.verb = "state",
		.session = AFB_SESSION_NONE,
		.callback = network_state,
		.info ="Return global network state"
	}, {
		.verb = "offline",
		.session = AFB_SESSION_NONE,
		.callback = network_offline,
		.info ="Return global network state"
	}, {
		.verb = "technologies",
		.session = AFB_SESSION_NONE,
		.callback = network_technologies,
		.info ="Return list of network technologies"
	}, {
		.verb = "services",
		.session = AFB_SESSION_NONE,
		.callback = network_services,
		.info ="Return list of network services"
	}, {
		.verb = "enable_technology",
		.session = AFB_SESSION_NONE,
		.callback = network_enable_technology,
		.info ="Enable a technology"
	}, {
		.verb = "disable_technology",
		.session = AFB_SESSION_NONE,
		.callback = network_disable_technology,
		.info ="Disable a technology"
	}, {
		.verb = "scan_services",
		.session = AFB_SESSION_NONE,
		.callback = network_scan_services,
		.info ="Scan services"
	}, {
		.verb = "move_service",
		.session = AFB_SESSION_NONE,
		.callback = network_move_service,
		.info ="Arrange service order"
	}, {
		.verb = "remove_service",
		.session = AFB_SESSION_NONE,
		.callback = network_remove_service,
		.info ="Remove service"
	}, {
		.verb = "reset_counters",
		.session = AFB_SESSION_NONE,
		.callback = network_reset_counters,
		.info ="Reset service counters"
	}, {
		.verb = "connect_service",
		.session = AFB_SESSION_NONE,
		.callback = network_connect_service,
		.info ="Connect service"
	}, {
		.verb = "disconnect_service",
		.session = AFB_SESSION_NONE,
		.callback = network_disconnect_service,
		.info ="Disconnect service"
	}, {
		.verb = "get_property",
		.session = AFB_SESSION_NONE,
		.callback = network_get_property,
		.info ="Get property"
	}, {
		.verb = "set_property",
		.session = AFB_SESSION_NONE,
		.callback = network_set_property,
		.info ="Set property"
	}, {
		.verb = "agent_response",
		.session = AFB_SESSION_NONE,
		.callback = network_agent_response,
		.info ="Agent response"
	},

	{ } /* marker for end of the array */
};

/*
 * description of the binding for afb-daemon
 */
const struct afb_binding_v2 afbBindingV2 = {
	.api = "network-manager", /* the API name (or binding name or prefix) */
	.specification = "networking API", /* short description of of the binding */
	.verbs = network_verbs, /* the array describing the verbs of the API */
	.init = init,
};
