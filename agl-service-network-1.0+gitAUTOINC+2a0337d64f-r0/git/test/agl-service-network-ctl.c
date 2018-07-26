/*
 * Copyright (C) 2018 Konsulko Group
 * Author Pantelis Antoniou <pantelis.antoniou@konsulko.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <pthread.h>
#include <ctype.h>
#include <alloca.h>
#include <assert.h>

#include <systemd/sd-event.h>
#include <json-c/json.h>

#include <afb/afb-wsj1.h>
#include <afb/afb-ws-client.h>
#include <afb/afb-proto-ws.h>

struct state {
	const char *url;
	int port;
	const char *token;
	const char *api;
	char *uri;
	bool interactive;
	bool debug;
	bool noexit;

	sd_event *loop;
	struct afb_wsj1 *wsj1;
	const char *proto;
	bool hangup;
};

struct cmd {
	const char *verb;
	int (*call)(struct state *s, const struct cmd *c, int argc, char *argv[]);
	void (*reply)(void *closure, struct afb_wsj1_msg *msg);
};

/* print usage of the program */
/* declaration of functions */
static void on_wsj1_hangup(void *closure, struct afb_wsj1 *wsj1);
static void on_wsj1_call(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg);
static void on_wsj1_event(void *closure, const char *event, struct afb_wsj1_msg *msg);

/* the callback interface for wsj1 */
static struct afb_wsj1_itf wsj1_itf = {
	.on_hangup = on_wsj1_hangup,
	.on_call = on_wsj1_call,
	.on_event = on_wsj1_event
};

static void on_reply(struct state *s, const char *verb, struct afb_wsj1_msg *msg)
{
	printf("ON-REPLY %s: %s: %s\n%s\n",
			s->api,
			verb,
			afb_wsj1_msg_is_reply_ok(msg) ? "OK" : "ERROR",
			json_object_to_json_string_ext(afb_wsj1_msg_object_j(msg),
						JSON_C_TO_STRING_PRETTY));
	fflush(stdout);

	/* if non interactive terminate */
	if (!s->interactive) {
		if (!s->noexit)
			sd_event_exit(s->loop,
				afb_wsj1_msg_is_reply_ok(msg) ? 0 : EXIT_FAILURE);
		else
			printf("Ctrl-C to exit\n");
	}

}

static int do_call(struct state *s, const struct cmd *c, json_object *jparams)
{
	printf("CALL %s(%s)\n", c->verb,
			jparams ? json_object_to_json_string(jparams) : "");

	return afb_wsj1_call_j(s->wsj1, s->api, c->verb, jparams, c->reply, s);
}

static int call_void(struct state *s, const struct cmd *c, int argc, char *argv[])
{
	return do_call(s, c, NULL);
}

static void on_ping_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "ping", msg);
}

static void on_subscribe_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "subscribe", msg);
}

static void on_unsubscribe_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "unsubscribe", msg);
}

static int call_subscribe_unsubscribe(struct state *s, const struct cmd *c, int argc, char *argv[])
{
	json_object *jobj;

	/* must give event name */
	if (argc < 1)
		return -1;
	jobj = json_object_new_object();
	json_object_object_add(jobj, "value", json_object_new_string(argv[0]));

	return do_call(s, c, jobj);
}

static void on_state_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "state", msg);
}

static void on_reset_counters_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "reset_counters", msg);
}

static void on_technologies_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "technologies", msg);
}

static void on_services_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "services", msg);
}

static void on_enable_technology_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "enable_technology", msg);
}

static void on_disable_technology_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "disable_technology", msg);
}

static int call_technology_arg(struct state *s, const struct cmd *c, int argc, char *argv[])
{
	json_object *jobj;

	if (argc < 1)
		return -1;

	jobj = json_object_new_object();
	json_object_object_add(jobj, "technology", json_object_new_string(argv[0]));

	return do_call(s, c, jobj);
}

static void on_scan_services_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "scan_services", msg);
}

static void on_offline_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "offline", msg);
}

static int call_offline(struct state *s, const struct cmd *c, int argc, char *argv[])
{
	json_object *jobj;

	/* with no arguments it's a getter */
	if (argc >= 1) {
		jobj = json_object_new_object();
		json_object_object_add(jobj, "value", json_object_new_string(argv[0]));
	} else
		jobj = NULL;

	return do_call(s, c, jobj);
}

static void on_connect_service_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "connect_service", msg);
}

static int call_service_arg(struct state *s, const struct cmd *c, int argc, char *argv[])
{
	json_object *jobj;

	if (argc < 1)
		return -1;

	jobj = json_object_new_object();
	json_object_object_add(jobj, "service", json_object_new_string(argv[0]));

	return do_call(s, c, jobj);
}

static void on_disconnect_service_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "disconnect_service", msg);
}

static void on_remove_service_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "remove_service", msg);
}

static void on_move_service_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "move_service", msg);
}

static int call_move_service(struct state *s, const struct cmd *c, int argc, char *argv[])
{
	json_object *jobj;

	/* 3 arguments and the middle must be before or after */
	if (argc < 3 || (strcmp(argv[1], "before") && strcmp(argv[1], "after")))
		return -1;

	jobj = json_object_new_object();
	json_object_object_add(jobj, "service", json_object_new_string(argv[0]));
	json_object_object_add(jobj,
			!strcmp(argv[1], "before") ? "before_service" : "after_service",
			json_object_new_string(argv[2]));

	return do_call(s, c, jobj);
}

static void on_set_property_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "set_property", msg);
}

static void on_get_property_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "get_property", msg);
}

static bool get_property_obj(json_object *jparent, int argc, char *argv[])
{
	json_object *jobj, *jobj2;
	int i, j, nest;

	for (i = 0; i < argc; i++) {

		if ((i < (argc - 1) && !strcmp(argv[i+1], "{"))) {
			nest = 1;
			for (j = i + 2; j < argc; j++) {
				if (!strcmp(argv[j], "{"))
					nest++;
				else if (!strcmp(argv[j], "}"))
					nest--;

				if (!nest)
					break;
			}
			if (j >= argc)
				return false;

			jobj = json_object_new_object();

			jobj2 = json_object_new_array();
			get_property_obj(jobj2, j - i - 2, argv + i + 2);
			json_object_object_add(jobj, argv[i], jobj2);
			i = j;
		} else
			jobj = json_object_new_string(argv[i]);

		json_object_array_add(jparent, jobj);

	}

	return true;
}

struct json_object *get_property_value(const char *str)
{
	char *le, *de;
	long long ll;
	double d;

	if (!strcmp(str, "null"))
		return NULL;
	if (!strcmp(str, "true"))
	       return json_object_new_boolean(true);
	if (!strcmp(str, "false"))
	       return json_object_new_boolean(false);

	/* try both double and long and see which one is furthest */
	ll = strtoll(str, &le, 0);
	d = strtod(str, &de);

	if (de > le) {
		while (isspace(*de))
			de++;
		if (!*de)
			return json_object_new_double(d);
	} else {
		while (isspace(*le))
			le++;
		if (!*le)
			return json_object_new_int64((int64_t)ll);
	}

	/* everything else is a string */
	return json_object_new_string(str);
}

/* how many args is a single json object value */
static int next_span(int pos, int argc, char *argv[])
{
	const char *left, *right;
	int j, nest;

	if (pos >= argc)
		return 0;

	if (!strcmp(argv[pos], "{") || !strcmp(argv[pos], "[")) {

		if (!strcmp(argv[pos], "{")) {
			left = "{";
			right = "}";
		} else {
			left = "[";
			right = "]";
		}
		nest = 1;
		for (j = pos + 1; nest > 0 && j < argc; j++) {
			if (!strcmp(argv[j], left))
				nest++;
			else if (!strcmp(argv[j], right))
				nest--;
		}
		if (nest && j >= argc) {
			fprintf(stderr, "nesting error\n");
			return -1;
		}

		return j - pos;
	}
	return 1;
}

bool add_property_value(json_object *jparent, const char *key, int argc, char *argv[])
{
	json_object *jobj;
	const char *key2;
	int i, span;

	if (!strcmp(argv[0], "[")) {
		assert(!strcmp(argv[argc - 1], "]"));

		argc -= 2;
		argv++;

		jobj = json_object_new_array();

		for (i = 0; i < argc; ) {
			span = next_span(i, argc, argv);
			if (span < 0) {
				fprintf(stderr, "bad nesting\n");
				return NULL;
			}

			if (!add_property_value(jobj, NULL, span, argv + i)) {
				fprintf(stderr, "error adding array\n");
				return false;
			}
			i += span;

		}
	} else if (!strcmp(argv[0], "{")) {
		assert(!strcmp(argv[argc - 1], "}"));

		argc -= 2;
		argv++;

		jobj = json_object_new_object();

		for (i = 0; i < argc; ) {
			key2 = argv[i];

			span = next_span(i + 1, argc, argv);
			if (span < 0) {
				fprintf(stderr, "bad nesting\n");
				return NULL;
			}

			if (!add_property_value(jobj, key2, span, argv + i + 1)) {
				fprintf(stderr, "error adding object\n");
				return false;
			}
			i += 1 + span;
		}
	} else {
		jobj = get_property_value(argv[0]);
	}

	if (key)
		json_object_object_add(jparent, key, jobj);
	else
		json_object_array_add(jparent, jobj);

	return true;
}

bool set_property_obj(json_object *jparent, int argc, char *argv[])
{
	int i, span;
	const char *key;

	for (i = 0; i < argc; ) {

		key = argv[i];
		if (!strcmp(key, "{") || !strcmp(key, "[")) {
			fprintf(stderr, "Bad object start {\n");
			return false;
		}
		i++;

		if (i > (argc - 1)) {
			fprintf(stderr, "out of arguments on key %s\n", key);
			return false;
		}

		span = next_span(i, argc, argv);
		if (span < 0) {
			fprintf(stderr, "bad nesting\n");
			return false;
		}

		if (!add_property_value(jparent, key, span, argv + i)) {
			fprintf(stderr, "error adding property\n");
			return false;
		}
		i += span;
	}

	return true;
}

static int call_get_set_property(struct state *s, const struct cmd *c, int argc, char *argv[])
{
	json_object *jobj, *jprop = NULL;
	const char *type;
	int min_argc;

	if (argc < 1)
		return -1;

	type = argv[0];
	/* global or technology or service */
	if (strcmp(type, "global") && strcmp(type, "technology") &&
			strcmp(type, "service")) {
		fprintf(stderr, "unknown property type %s\n", type);
		return -1;
	}

	/* minimum is get global */
	min_argc = 1;

	/* technology or service have extra argument */
	if (!strcmp(type, "technology") || !strcmp(type, "service"))
		min_argc++;

	/* set property must have an argument */
	min_argc += !strcmp(c->verb, "set_property") ? 2 : 0;

	if (argc < min_argc) {
		fprintf(stderr, "not enough arguments\n");
		return -1;
	}

	jobj = json_object_new_object();

	argc--;
	argv++;

	if (!strcmp(type, "technology")) {
		json_object_object_add(jobj, "technology",
				json_object_new_string(argv[0]));
		argc--;
		argv++;
	} else if (!strcmp(type, "service")) {
		json_object_object_add(jobj, "service",
				json_object_new_string(argv[0]));
		argc--;
		argv++;
	}

	/* get is array, set is object */
	if (!strcmp(c->verb, "get_property")) {
		if (argc > 0) {
			jprop = json_object_new_array();
			get_property_obj(jprop, argc, argv);
		}
	} else {
		jprop = json_object_new_object();
		set_property_obj(jprop, argc, argv);
	}

	if (jprop)
		json_object_object_add(jobj, "properties", jprop);

	return do_call(s, c, jobj);
}

static void on_agent_response_reply(void *closure, struct afb_wsj1_msg *msg)
{
	on_reply(closure, "agent_response", msg);
}

static int call_agent_response(struct state *s, const struct cmd *c, int argc, char *argv[])
{
	json_object *jresp = NULL, *jprop = NULL;
	const char *method;
	const char *propname;
	int id;

	if (argc < 3)
		return -1;

	method = *argv++;
	argc--;

	id = (int)strtol(*argv++, NULL, 10);
	argc--;

	if (id <= 0) {
		fprintf(stderr, "bad agent response id %d\n", id);
		return -1;
	}

	propname = NULL;
	if (!strcmp(method, "request-input")) {
		propname = "fields";
	} else {
		fprintf(stderr, "unknown agent response method '%s'\n",
			method);
		return -1;
	}

	jresp = json_object_new_object();
	json_object_object_add(jresp, "method",
			json_object_new_string(method));
	json_object_object_add(jresp, "id",
			json_object_new_int(id));

	if (argc > 0) {
		jprop = json_object_new_object();
		set_property_obj(jprop, argc, argv);
		json_object_object_add(jresp, propname, jprop);
	}

	return do_call(s, c, jresp);
}


static const struct cmd cmds[] = {
	{
		.verb	= "ping",
		.call	= call_void,
		.reply	= on_ping_reply,
	}, {
		.verb	= "subscribe",
		.call	= call_subscribe_unsubscribe,
		.reply	= on_subscribe_reply,
	}, {
		.verb	= "unsubscribe",
		.call	= call_subscribe_unsubscribe,
		.reply	= on_unsubscribe_reply,
	}, {
		.verb	= "state",
		.call	= call_void,
		.reply	= on_state_reply,
	}, {
		.verb	= "reset_counters",
		.call	= call_service_arg,
		.reply	= on_reset_counters_reply,
	}, {
		.verb	= "technologies",
		.call	= call_void,
		.reply	= on_technologies_reply,
	}, {
		.verb	= "services",
		.call	= call_void,
		.reply	= on_services_reply,
	}, {
		.verb	= "enable_technology",
		.call	= call_technology_arg,
		.reply	= on_enable_technology_reply,
	}, {
		.verb	= "disable_technology",
		.call	= call_technology_arg,
		.reply	= on_disable_technology_reply,
	}, {
		.verb	= "scan_services",
		.call	= call_technology_arg,
		.reply	= on_scan_services_reply,
	}, {
		.verb	= "offline",
		.call	= call_offline,
		.reply	= on_offline_reply,
	}, {
		.verb	= "connect_service",
		.call	= call_service_arg,
		.reply	= on_connect_service_reply,
	}, {
		.verb	= "disconnect_service",
		.call	= call_service_arg,
		.reply	= on_disconnect_service_reply,
	}, {
		.verb	= "remove_service",
		.call	= call_service_arg,
		.reply	= on_remove_service_reply,
	}, {
		.verb	= "move_service",
		.call	= call_move_service,
		.reply	= on_move_service_reply,
	}, {
		.verb	= "set_property",
		.call	= call_get_set_property,
		.reply	= on_set_property_reply,
	}, {
		.verb	= "get_property",
		.call	= call_get_set_property,
		.reply	= on_get_property_reply,
	}, {
		.verb	= "agent_response",
		.call	= call_agent_response,
		.reply	= on_agent_response_reply,
	},
	{ }
};

static int call_cmd(struct state *s, int argc, char *argv[])
{
	const struct cmd *c;
	const char *verb;

	/* first argument is verb */
	if (argc < 1 || !argv[0]) {
		if (!s->interactive)
			fprintf(stderr, "No verb given\n");
		goto out_err;
	}
	verb = argv[0];
	argv++;
	argc--;

	for (c = cmds; c->verb; c++) {
		if (!strcmp(verb, c->verb))
			return c->call(s, c, argc, argv);
	}

	if (!s->interactive)
		fprintf(stderr, "Unknown API verb \"%s\"\n", verb);
out_err:

	if (!s->interactive)
		sd_event_exit(s->loop, EXIT_FAILURE);

	return -1;
}

static struct option opts[] = {
	{ "url",	required_argument,	0, 'u' },
	{ "port",	required_argument,	0, 'p' },
	{ "token",	required_argument,	0, 't' },
	{ "api",	required_argument,	0, 'a' },
	{ "noexit",	no_argument,		0, 'x' },
	{ "debug",	no_argument,		0, 'd' },
	{ "help",	no_argument,		0, 'h' },
	{ }
};

#define DEFAULT_URL	"ws://localhost"
#define DEFAULT_PORT	1234
#define DEFAULT_TOKEN	"1"
#define DEFAULT_API	"network-manager"
#define DEFAULT_DEBUG	true

static void usage(int status, const char *arg0)
		__attribute__((__noreturn__));

static void usage(int status, const char *arg0)
{
	const char *name = strrchr(arg0, '/');
	FILE *outf = status ? stderr : stdout;

	name = name ? name + 1 : arg0;

	fprintf(outf, "usage: %s <options> [arguments]\n", name);
	fprintf(outf, "  options are:\n");
	fprintf(outf, "    -u, --url=X          URL to connect to (default %s)\n", DEFAULT_URL);
	fprintf(outf, "    -p, --port=X         Port to use for connection (default %d)\n", DEFAULT_PORT);
	fprintf(outf, "    -t, --token=X        Token to use (default %s)\n", DEFAULT_TOKEN);
	fprintf(outf, "    -a, --api=X          API to use (default %s)\n", DEFAULT_API);
	fprintf(outf, "    -x, --noexit         Do not exit in non-interactive mode (events)\n");
	fprintf(outf, "    -d, --debug          Enable debug printouts\n");
	fprintf(outf, "    -h, -?, --help       Display this help\n");

	exit(status);
}

/* entry function */
int main(int argc, char **argv)
{
	int rc, ret, cc = 0, option_index;
	const char *name;
	struct state state, *s = &state;

	memset(s, 0, sizeof(*s));
	s->url = DEFAULT_URL;
	s->port = DEFAULT_PORT;
	s->token = DEFAULT_TOKEN;
	s->api = DEFAULT_API;
	s->debug = DEFAULT_DEBUG;

	s->interactive = false;
	s->noexit = false;

	while ((cc = getopt_long(argc, argv, "u:p:t:a:xdh?", opts,
					&option_index)) != -1) {

		if (cc == 0 && option_index >= 0) {
			name = opts[option_index].name;
			if (!name)
				continue;
			/* we don't have long options without short ones */
			usage(EXIT_FAILURE, argv[0]);
		}

		switch (cc) {
		case 'u':
			s->url = optarg;
			break;
		case 'p':
			s->port = atoi(optarg);
			break;
		case 't':
			s->token = optarg;
			break;
		case 'a':
			s->api = optarg;
			break;
		case 'x':
			s->noexit = true;
			break;
		case 'd':
			s->debug = true;
			break;
		case 'h':
		case '?':
			usage(0, argv[0]);
			break;
		}
	}

	/* no arguments left, interactive mode */
	if (optind >= argc) {
		printf("optind=%d\n", optind);
		printf("argc=%d\n", argc);
		s->interactive = true;
	}

	ret = EXIT_FAILURE;

	rc = asprintf(&s->uri, "%s:%d/api?token=%s", s->url, s->port, s->token);
	if (rc < 0) {
		fprintf(stderr, "Unable to build URI\n");
		goto out;
	}

	if (s->debug) {
		fprintf(stderr, "URI:         \"%s\"\n", s->uri);
		fprintf(stderr, "API:         \"%s\"\n", s->api);
		fprintf(stderr, "interactive: %s\n", s->interactive ? "true" : "false");
		fprintf(stderr, "noexit:      %s\n", s->noexit ? "true" : "false");
	}

	/* get the default event loop */
	rc = sd_event_default(&s->loop);
	if (rc < 0) {
		fprintf(stderr, "connection to default event loop failed: %s\n", strerror(-rc));
		goto out_no_event;
	}

	/* connect the websocket wsj1 to the uri given by the first argument */
	s->wsj1 = afb_ws_client_connect_wsj1(s->loop, s->uri, &wsj1_itf, s);
	if (s->wsj1 == NULL) {
		fprintf(stderr, "connection to %s failed: %m\n", argv[1]);
		goto out_no_wsj1;
	}

	if (!s->interactive) {
		ret = call_cmd(s, argc - optind, argv + optind);
		if (ret < 0) {
			fprintf(stderr, "command failed\n");
			sd_event_exit(s->loop, EXIT_FAILURE);
		}
	} else {
		fprintf(stderr, "interactive mode not yet supported\n");
		sd_event_exit(s->loop, EXIT_FAILURE);
	}

	ret = sd_event_loop(s->loop);

	/* cleanup */

	afb_wsj1_unref(s->wsj1);
out_no_wsj1:
	sd_event_unref(s->loop);
out_no_event:
	free(s->uri);
out:
	return ret;
}

/* called when wsj1 hangsup */
static void on_wsj1_hangup(void *closure, struct afb_wsj1 *wsj1)
{
	struct state *s = closure;

	printf("ON-HANGUP\n");
	fflush(stdout);

	s->hangup = true;
	sd_event_exit(s->loop, 0);
}

/* called when wsj1 receives a method invocation */
static void on_wsj1_call(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg)
{
	int rc;

	printf("ON-CALL %s/%s:\n%s\n", api, verb,
			json_object_to_json_string_ext(afb_wsj1_msg_object_j(msg),
						JSON_C_TO_STRING_PRETTY));
	fflush(stdout);
	rc = afb_wsj1_reply_error_s(msg, "\"unimplemented\"", NULL);
	if (rc < 0)
		fprintf(stderr, "replying failed: %m\n");
}

/* called when wsj1 receives an event */
static void on_wsj1_event(void *closure, const char *event, struct afb_wsj1_msg *msg)
{
	printf("ON-EVENT %s:\n%s\n", event,
			json_object_to_json_string_ext(afb_wsj1_msg_object_j(msg),
						JSON_C_TO_STRING_PRETTY));
	fflush(stdout);
}
