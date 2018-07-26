var afb = new AFB("api", "HELLO");
var ws;

function add_debbug_panel() {
	
	if (document.getElementById("debug-panel"))
		return;

	var itm = document.getElementById("debug-panel-container");
	if (itm)
	{
		var pnl =
			"<div id=\"debug-panel\" class=\"expanded\">\n" +
			"   <button id=\"debug-panel-collapse\" onclick=\"debug_panel_collapse();\">&gt;</button>\n" +
			"    <button id=\"debug-panel-expand\" onclick=\"debug_panel_expand();\">&lt;</button>\n" +
			"    <div id=\"debug-panel-content\">\n" +
			"        <h1>Debug</h1>\n" +
			"        <h2>Call</h2><div id=\"debug-panel-call\">\n" +
			"            <ul>\n" +
			"                <li><strong>api : </strong><span id=\"debug-panel-call-id\"></span></li>\n" +
			"                <li><strong>verb : </strong><span id=\"debug-panel-call-verb\"></span></li>\n" +
			"                <li><strong>query : </strong></li>\n" +
			"            </ul>\n" +
			"            <pre id=\"debug-panel-call-query\"></pre>\n" +
			"        </div>\n" +
			"        <h2>Response</h2><pre id=\"debug-panel-response\"></pre>\n" +
			"        <h2>Event</h2><pre id=\"debug-panel-event\"></pre>\n" +
			"    </div>\n" +
			"</div>\n";
		itm.insertAdjacentHTML("afterbegin", pnl);
	}
}

function createClass(name,rules) {
	var style = document.createElement('style');
	style.type = 'text/css';
	document.getElementsByTagName('head')[0].appendChild(style);
	if(!(style.sheet||{}).insertRule) 
		(style.styleSheet || style.sheet).addRule(name, rules);
	else
		style.sheet.insertRule(name+"{"+rules+"}",0);
}

function syntaxHighlight(json) {
	if (typeof json != 'string')
		json = JSON.stringify(json, undefined, 2);
	
	json = json.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
	return json.replace(/("(\\u[a-zA-Z0-9]{4}|\\[^u]|[^\\"])*"(\s*:)?|\b(true|false|null)\b|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?)/g, function (match) {
		var cls = 'json-number';
		if (/^"/.test(match)) {
			if (/:$/.test(match)) {
				cls = 'json-key';
			} else {
				cls = 'json-string';
			}
		} else if (/true|false/.test(match)) {
			cls = 'json-boolean';
		} else if (/null/.test(match)) {
			cls = 'json-null';
		}
		return '<span class="' + cls + '">' + match + '</span>';
	});
}

function set_item_html(id, text)
{
	var itm = document.getElementById(id);
	if (itm) itm.innerHTML = text;
}

function set_item_text(id, text)
{
	var itm = document.getElementById(id);
	if (itm) itm.innerText = text;
}

function debug_panel_collapse() {
	var pnl = document.getElementById('debug-panel');
	if (pnl)
	{
		pnl.classList.remove('expanded');
		pnl.classList.add('collapsed');
	}
}

function debug_panel_expand() {
	var pnl = document.getElementById('debug-panel');
	if (pnl)
	{
		pnl.classList.remove('collapsed');
		pnl.classList.add('expanded');
	}
}

function init() {
	add_debbug_panel();
	ws = new afb.ws(onopen, onabort);
}

function onopen() {
	//callbinder("ll-auth", "getuser", "");
	ws.onevent("*", gotevent);
}

function onabort() {
}

function replyok(obj) {
	console.log("replyok:" + JSON.stringify(obj));
	set_item_html("debug-panel-response", syntaxHighlight(JSON.stringify(obj, null, 4)));
}

function replyerr(obj) {
	console.log("replyerr:" + JSON.stringify(obj));
	set_item_html("debug-panel-response", syntaxHighlight(JSON.stringify(obj, null, 4)));
}

function gotevent(obj) {
	console.log("gotevent:" + JSON.stringify(obj));
	set_item_html("debug-panel-event", syntaxHighlight(JSON.stringify(obj, null, 4)));
}

function callbinder(api, verb, query) {
	console.log ("subscribe api="+api+" verb="+verb+" query=" +query);

	set_item_text("debug-panel-call-api", api);
	set_item_text("debug-panel-call-verb", verb);
	set_item_html("debug-panel-call-query", syntaxHighlight(JSON.stringify(query, null, 4)));

	ws.call(api+"/"+verb, query).then(replyok, replyerr);
}

function subscribe() {
	callbinder("identity", "subscribe", {});
}

function getIdentity() {
	callbinder("identity", "get", {});
}

function logout() {
	callbinder("identity", "logout", {});
}

function fakeAuth() {

	var e = document.getElementById("fake-auth-kind");
	var arg = {
		"kind": e.options[e.selectedIndex].value,
		"key": (document.getElementById("fake-auth-key").value === "custom" ? document.getElementById("fake-auth-key-custom").value : document.getElementById("fake-auth-key").value)
	}
	callbinder("identity", "fake-auth", arg);
}

function updateNfcUi() {
	document.getElementById('fake-auth-key-custom').disabled = !(document.getElementById("fake-auth-key").value === "custom");
}
