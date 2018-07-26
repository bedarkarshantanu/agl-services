
static const char _afb_description_v2_signal_composer[] =
    "{\"openapi\":\"3.0.0\",\"$schema\":\"http://iot.bzh/download/openapi/sch"
    "ema-3.0/default-schema.json\",\"info\":{\"description\":\"\",\"title\":\""
    "signals-composer-service\",\"version\":\"4.0\",\"x-binding-c-generator\""
    ":{\"api\":\"signal-composer\",\"version\":2,\"prefix\":\"\",\"postfix\":"
    "\"\",\"start\":null,\"onevent\":\"onEvent\",\"preinit\":\"loadConf\",\"i"
    "nit\":\"execConf\",\"scope\":\"\",\"private\":false}},\"servers\":[{\"ur"
    "l\":\"ws://{host}:{port}/api/monitor\",\"description\":\"Signals compose"
    "r API connected to low level AGL services\",\"variables\":{\"host\":{\"d"
    "efault\":\"localhost\"},\"port\":{\"default\":\"1234\"}},\"x-afb-events\""
    ":[{\"$ref\":\"#/components/schemas/afb-event\"}]}],\"components\":{\"sch"
    "emas\":{\"afb-reply\":{\"$ref\":\"#/components/schemas/afb-reply-v2\"},\""
    "afb-event\":{\"$ref\":\"#/components/schemas/afb-event-v2\"},\"afb-reply"
    "-v2\":{\"title\":\"Generic response.\",\"type\":\"object\",\"required\":"
    "[\"jtype\",\"request\"],\"properties\":{\"jtype\":{\"type\":\"string\",\""
    "const\":\"afb-reply\"},\"request\":{\"type\":\"object\",\"required\":[\""
    "status\"],\"properties\":{\"status\":{\"type\":\"string\"},\"info\":{\"t"
    "ype\":\"string\"},\"token\":{\"type\":\"string\"},\"uuid\":{\"type\":\"s"
    "tring\"},\"reqid\":{\"type\":\"string\"}}},\"response\":{\"type\":\"obje"
    "ct\"}}},\"afb-event-v2\":{\"type\":\"object\",\"required\":[\"jtype\",\""
    "event\"],\"properties\":{\"jtype\":{\"type\":\"string\",\"const\":\"afb-"
    "event\"},\"event\":{\"type\":\"string\"},\"data\":{\"type\":\"object\"}}"
    "}},\"x-permissions\":{\"addObjects\":{\"permission\":\"urn:AGL:permissio"
    "n::platform:composer:addObjects\"}},\"responses\":{\"200\":{\"descriptio"
    "n\":\"A complex object array response\",\"content\":{\"application/json\""
    ":{\"schema\":{\"$ref\":\"#/components/schemas/afb-reply\"}}}}}},\"paths\""
    ":{\"/subscribe\":{\"description\":\"Subscribe to a signal object\",\"par"
    "ameters\":[{\"in\":\"query\",\"name\":\"event\",\"required\":false,\"sch"
    "ema\":{\"type\":\"string\"}}],\"responses\":{\"200\":{\"$ref\":\"#/compo"
    "nents/responses/200\"}}},\"/unsubscribe\":{\"description\":\"Unsubscribe"
    " previously suscribed signal objects.\",\"parameters\":[{\"in\":\"query\""
    ",\"name\":\"event\",\"required\":false,\"schema\":{\"type\":\"string\"}}"
    "],\"responses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}},\"/"
    "get\":{\"description\":\"Get informations about a resource or element\","
    "\"responses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}},\"/li"
    "st\":{\"description\":\"List all signals already configured\",\"response"
    "s\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}},\"/addObjects\""
    ":{\"description\":\"Load new objects from an additional config file desi"
    "gnated by JSON argument with the key 'file'.\",\"get\":{\"x-permissions\""
    ":{\"$ref\":\"#/components/x-permissions/addObjects\"},\"responses\":{\"2"
    "00\":{\"$ref\":\"#/components/responses/200\"}}},\"parameters\":[{\"in\""
    ":\"query\",\"name\":\"path\",\"required\":true,\"schema\":{\"type\":\"st"
    "ring\"}}]}}}"
;
#ifdef __cplusplus
#include <afb/afb-binding>
#endif
static const struct afb_auth _afb_auths_v2_signal_composer[] = {
	afb::auth_permission("urn:AGL:permission::platform:composer:addObjects")
};

 void subscribe(struct afb_req req);
 void unsubscribe(struct afb_req req);
 void get(struct afb_req req);
 void list(struct afb_req req);
 void addObjects(struct afb_req req);

static const struct afb_verb_v2 _afb_verbs_v2_signal_composer[] = {
    {
        .verb = "subscribe",
        .callback = subscribe,
        .auth = NULL,
        .info = "Subscribe to a signal object",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "unsubscribe",
        .callback = unsubscribe,
        .auth = NULL,
        .info = "Unsubscribe previously suscribed signal objects.",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "get",
        .callback = get,
        .auth = NULL,
        .info = "Get informations about a resource or element",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "list",
        .callback = list,
        .auth = NULL,
        .info = "List all signals already configured",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "addObjects",
        .callback = addObjects,
        .auth = &_afb_auths_v2_signal_composer[0],
        .info = "Load new objects from an additional config file designated by JSON argument with the key 'file'.",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = NULL,
        .callback = NULL,
        .auth = NULL,
        .info = NULL,
        .session = 0
	}
};

const struct afb_binding_v2 afbBindingV2 = {
    .api = "signal-composer",
    .specification = _afb_description_v2_signal_composer,
    .info = "",
    .verbs = _afb_verbs_v2_signal_composer,
    .preinit = loadConf,
    .init = execConf,
    .onevent = onEvent,
    .noconcurrency = 0
};

