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

#ifndef NETWORK_API_H
#define NETWORK_API_H

#include <alloca.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <glib.h>

#include <json-c/json.h>

#define CONNMAN_SERVICE                 	"net.connman"
#define CONNMAN_MANAGER_INTERFACE		CONNMAN_SERVICE ".Manager"
#define CONNMAN_TECHNOLOGY_INTERFACE		CONNMAN_SERVICE ".Technology"
#define CONNMAN_SERVICE_INTERFACE		CONNMAN_SERVICE ".Service"
#define CONNMAN_PROFILE_INTERFACE		CONNMAN_SERVICE ".Profile"
#define CONNMAN_COUNTER_INTERFACE		CONNMAN_SERVICE ".Counter"
#define CONNMAN_ERROR_INTERFACE			CONNMAN_SERVICE ".Error"
#define CONNMAN_AGENT_INTERFACE			CONNMAN_SERVICE ".Agent"

#define CONNMAN_MANAGER_PATH			"/"
#define CONNMAN_PATH				"/net/connman"
#define CONNMAN_TECHNOLOGY_PREFIX		CONNMAN_PATH "/technology"
#define CONNMAN_SERVICE_PREFIX			CONNMAN_PATH "/service"

#define CONNMAN_TECHNOLOGY_PATH(_t) \
    ({ \
     const char *__t = (_t); \
     size_t __len = strlen(CONNMAN_TECHNOLOGY_PREFIX) + 1 + \
     strlen(__t) + 1; \
     char *__tpath; \
     __tpath = alloca(__len + 1 + 1); \
     snprintf(__tpath, __len + 1, \
             CONNMAN_TECHNOLOGY_PREFIX "/%s", __t); \
             __tpath; \
     })

#define CONNMAN_SERVICE_PATH(_s) \
    ({ \
     const char *__s = (_s); \
     size_t __len = strlen(CONNMAN_SERVICE_PREFIX) + 1 + \
     strlen(__s) + 1; \
     char *__spath; \
     __spath = alloca(__len + 1 + 1); \
     snprintf(__spath, __len + 1, \
             CONNMAN_SERVICE_PREFIX "/%s", __s); \
             __spath; \
     })

#define AGENT_PATH				"/net/connman/Agent"
#define AGENT_SERVICE				"org.agent"

#define DBUS_REPLY_TIMEOUT			(120 * 1000)
#define DBUS_REPLY_TIMEOUT_SHORT		(10 * 1000)

#define CONNMAN_AT_MANAGER			"manager"
#define CONNMAN_AT_TECHNOLOGY			"technology"
#define CONNMAN_AT_SERVICE			"service"

struct network_state;

static inline const char *connman_strip_path(const char *path)
{
	const char *basename;

	basename = strrchr(path, '/');
	if (!basename)
		return NULL;
	basename++;
	/* at least one character */
	return *basename ? basename : NULL;
}

const struct property_info *connman_get_property_info(
		const char *access_type, GError **error);

gboolean connman_property_dbus2json(const char *access_type,
		json_object *jprop, const gchar *key, GVariant *var,
		gboolean *is_config,
		GError **error);

GVariant *connman_call(struct network_state *ns,
		const char *access_type, const char *type_arg,
		const char *method, GVariant *params, GError **error);

json_object *connman_get_properties(struct network_state *ns,
		const char *access_type, const char *type_arg,
		GError **error);

json_object *connman_get_property(struct network_state *ns,
		const char *access_type, const char *type_arg,
		gboolean is_json_name, const char *name, GError **error);

gboolean connman_set_property(struct network_state *ns,
		const char *access_type, const char *type_arg,
		gboolean is_json_name, const char *name, json_object *jval,
		GError **error);

/* convenience access methods */
static inline gboolean manager_property_dbus2json(json_object *jprop,
		const gchar *key, GVariant *var, gboolean *is_config,
		GError **error)
{
	return connman_property_dbus2json(CONNMAN_AT_MANAGER,
			jprop, key, var, is_config, error);
}

static inline gboolean technology_property_dbus2json(json_object *jprop,
		const gchar *key, GVariant *var, gboolean *is_config,
		GError **error)
{
	return connman_property_dbus2json(CONNMAN_AT_TECHNOLOGY,
			jprop, key, var, is_config, error);
}

static inline gboolean service_property_dbus2json(json_object *jprop,
		const gchar *key, GVariant *var, gboolean *is_config,
		GError **error)
{
	return connman_property_dbus2json(CONNMAN_AT_SERVICE,
			jprop, key, var, is_config, error);
}

static inline GVariant *manager_call(struct network_state *ns,
		const char *method, GVariant *params, GError **error)
{
	return connman_call(ns, CONNMAN_AT_MANAGER, NULL,
			method, params, error);
}

static inline GVariant *technology_call(struct network_state *ns,
		const char *technology, const char *method,
		GVariant *params, GError **error)
{
	return connman_call(ns, CONNMAN_AT_TECHNOLOGY, technology,
			method, params, error);
}

static inline GVariant *service_call(struct network_state *ns,
		const char *service, const char *method,
		GVariant *params, GError **error)
{
	return connman_call(ns, CONNMAN_AT_SERVICE, service,
			method, params, error);
}

static inline json_object *manager_properties(struct network_state *ns, GError **error)
{
	return connman_get_properties(ns,
			CONNMAN_AT_MANAGER, NULL, error);
}

static inline json_object *technology_properties(struct network_state *ns,
		GError **error, const gchar *technology)
{
	return connman_get_properties(ns,
			CONNMAN_AT_TECHNOLOGY, technology, error);
}

static inline json_object *service_properties(struct network_state *ns,
		GError **error, const gchar *service)
{
	return connman_get_properties(ns,
			CONNMAN_AT_SERVICE, service, error);
}

static inline json_object *manager_get_property(struct network_state *ns,
		gboolean is_json_name, const char *name, GError **error)
{
	return connman_get_property(ns, CONNMAN_AT_MANAGER, NULL,
			is_json_name, name, error);
}

static inline json_object *technology_get_property(struct network_state *ns,
		const char *technology,
		gboolean is_json_name, const char *name, GError **error)
{
	return connman_get_property(ns, CONNMAN_AT_TECHNOLOGY, technology,
			is_json_name, name, error);
}

static inline json_object *service_get_property(struct network_state *ns,
		const char *service,
		gboolean is_json_name, const char *name, GError **error)
{
	return connman_get_property(ns, CONNMAN_AT_SERVICE, service,
			is_json_name, name, error);
}

static inline gboolean manager_set_property(struct network_state *ns,
		gboolean is_json_name, const char *name, json_object *jval,
		GError **error)
{
	return connman_set_property(ns, CONNMAN_AT_MANAGER, NULL,
				    is_json_name, name, jval, error);

}

static inline gboolean technology_set_property(struct network_state *ns,
		const char *technology,
		gboolean is_json_name, const char *name, json_object *jval,
		GError **error)
{
	return connman_set_property(ns, CONNMAN_AT_TECHNOLOGY, technology,
				    is_json_name, name, jval, error);
}

static inline gboolean service_set_property(struct network_state *ns,
		const char *service,
		gboolean is_json_name, const char *name, json_object *jval,
		GError **error)
{
	return connman_set_property(ns, CONNMAN_AT_SERVICE, service,
				    is_json_name, name, jval, error);
}

struct connman_pending_work {
	struct network_state *ns;
	void *user_data;
	GCancellable *cancel;
	void (*callback)(void *user_data, GVariant *result, GError **error);
};

void connman_cancel_call(struct network_state *ns,
		struct connman_pending_work *cpw);

struct connman_pending_work *
connman_call_async(struct network_state *ns,
		const char *access_type, const char *type_arg,
		const char *method, GVariant *params, GError **error,
		void (*callback)(void *user_data, GVariant *result, GError **error),
		void *user_data);

void connman_decode_call_error(struct network_state *ns,
		const char *access_type, const char *type_arg,
		const char *method,
		GError **error);

#endif /* NETWORK_API_H */
