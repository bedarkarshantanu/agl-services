
static const char _afb_description_v2_UNICENS[] =
    "{\"openapi\":\"3.0.0\",\"$schema\":\"http:iot.bzh/download/openapi/schem"
    "a-3.0/default-schema.json\",\"info\":{\"description\":\"\",\"title\":\"u"
    "cs2\",\"version\":\"1.0\",\"x-binding-c-generator\":{\"api\":\"UNICENS\""
    ",\"version\":2,\"prefix\":\"ucs2_\",\"postfix\":\"\",\"start\":null,\"on"
    "event\":null,\"preinit\":null,\"init\":\"ucs2_initbinding\",\"scope\":\""
    "\",\"private\":false}},\"servers\":[{\"url\":\"ws://{host}:{port}/api/mo"
    "nitor\",\"description\":\"Unicens2 API.\",\"variables\":{\"host\":{\"def"
    "ault\":\"localhost\"},\"port\":{\"default\":\"1234\"}},\"x-afb-events\":"
    "[{\"$ref\":\"#/components/schemas/afb-event\"}]}],\"components\":{\"sche"
    "mas\":{\"afb-reply\":{\"$ref\":\"#/components/schemas/afb-reply-v2\"},\""
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
    "}},\"x-permissions\":{\"config\":{\"permission\":\"urn:AGL:permission:UN"
    "ICENS:public:initialise\"},\"monitor\":{\"permission\":\"urn:AGL:permiss"
    "ion:UNICENS:public:monitor\"},\"controller\":{\"permission\":\"urn:AGL:p"
    "ermission:UNICENS:public:controller\"}},\"responses\":{\"200\":{\"descri"
    "ption\":\"A complex object array response\",\"content\":{\"application/j"
    "son\":{\"schema\":{\"$ref\":\"#/components/schemas/afb-reply\"}}}}}},\"p"
    "aths\":{\"/listconfig\":{\"description\":\"List Config Files\",\"get\":{"
    "\"x-permissions\":{\"$ref\":\"#/components/x-permissions/config\"},\"par"
    "ameters\":[{\"in\":\"query\",\"name\":\"cfgpath\",\"required\":false,\"s"
    "chema\":{\"type\":\"string\"}}],\"responses\":{\"200\":{\"$ref\":\"#/com"
    "ponents/responses/200\"}}}},\"/initialise\":{\"description\":\"configure"
    " Unicens2 lib from NetworkConfig.XML.\",\"get\":{\"x-permissions\":{\"$r"
    "ef\":\"#/components/x-permissions/config\"},\"parameters\":[{\"in\":\"qu"
    "ery\",\"name\":\"filename\",\"required\":true,\"schema\":{\"type\":\"str"
    "ing\"}}],\"responses\":{\"200\":{\"$ref\":\"#/components/responses/200\""
    "}}}},\"/subscribe\":{\"description\":\"Subscribe to network events.\",\""
    "get\":{\"x-permissions\":{\"$ref\":\"#/components/x-permissions/monitor\""
    "},\"responses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}}},\""
    "/subscriberx\":{\"description\":\"Subscribe to Rx control message events"
    ".\",\"get\":{\"x-permissions\":{\"$ref\":\"#/components/x-permissions/mo"
    "nitor\"},\"responses\":{\"200\":{\"$ref\":\"#/components/responses/200\""
    "}}}},\"/writei2c\":{\"description\":\"Writes I2C command to remote node."
    "\",\"get\":{\"x-permissions\":{\"$ref\":\"#/components/x-permissions/mon"
    "itor\"},\"parameters\":[{\"in\":\"query\",\"name\":\"node\",\"required\""
    ":true,\"schema\":{\"type\":\"integer\",\"format\":\"int32\"}},{\"in\":\""
    "query\",\"name\":\"data\",\"required\":true,\"schema\":{\"type\":\"array"
    "\",\"format\":\"int32\"},\"style\":\"simple\"}],\"responses\":{\"200\":{"
    "\"$ref\":\"#/components/responses/200\"}}}},\"/sendmessage\":{\"descript"
    "ion\":\"Transmits a control message to a node.\",\"get\":{\"x-permission"
    "s\":{\"$ref\":\"#/components/x-permissions/controller\"},\"parameters\":"
    "[{\"in\":\"query\",\"name\":\"node\",\"required\":true,\"schema\":{\"typ"
    "e\":\"integer\",\"format\":\"int32\"}},{\"in\":\"query\",\"name\":\"msgi"
    "d\",\"required\":true,\"schema\":{\"type\":\"integer\",\"format\":\"int3"
    "2\"}},{\"in\":\"query\",\"name\":\"data\",\"required\":false,\"schema\":"
    "{\"type\":\"string\",\"format\":\"byte\"},\"style\":\"simple\"}],\"respo"
    "nses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}}}}}"
;

static const struct afb_auth _afb_auths_v2_UNICENS[] = {
	{ .type = afb_auth_Permission, .text = "urn:AGL:permission:UNICENS:public:initialise" },
	{ .type = afb_auth_Permission, .text = "urn:AGL:permission:UNICENS:public:monitor" },
	{ .type = afb_auth_Permission, .text = "urn:AGL:permission:UNICENS:public:controller" }
};

 void ucs2_listconfig(struct afb_req req);
 void ucs2_initialise(struct afb_req req);
 void ucs2_subscribe(struct afb_req req);
 void ucs2_subscriberx(struct afb_req req);
 void ucs2_writei2c(struct afb_req req);
 void ucs2_sendmessage(struct afb_req req);

static const struct afb_verb_v2 _afb_verbs_v2_UNICENS[] = {
    {
        .verb = "listconfig",
        .callback = ucs2_listconfig,
        .auth = &_afb_auths_v2_UNICENS[0],
        .info = "List Config Files",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "initialise",
        .callback = ucs2_initialise,
        .auth = &_afb_auths_v2_UNICENS[0],
        .info = "configure Unicens2 lib from NetworkConfig.XML.",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "subscribe",
        .callback = ucs2_subscribe,
        .auth = &_afb_auths_v2_UNICENS[1],
        .info = "Subscribe to network events.",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "subscriberx",
        .callback = ucs2_subscriberx,
        .auth = &_afb_auths_v2_UNICENS[1],
        .info = "Subscribe to Rx control message events.",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "writei2c",
        .callback = ucs2_writei2c,
        .auth = &_afb_auths_v2_UNICENS[1],
        .info = "Writes I2C command to remote node.",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "sendmessage",
        .callback = ucs2_sendmessage,
        .auth = &_afb_auths_v2_UNICENS[2],
        .info = "Transmits a control message to a node.",
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
    .api = "UNICENS",
    .specification = _afb_description_v2_UNICENS,
    .info = "",
    .verbs = _afb_verbs_v2_UNICENS,
    .preinit = NULL,
    .init = ucs2_initbinding,
    .onevent = NULL,
    .noconcurrency = 0
};

