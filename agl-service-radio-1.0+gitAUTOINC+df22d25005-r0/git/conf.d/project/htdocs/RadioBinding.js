    var afb = new AFB("api", "mysecret");
    var ws;
    var sndcard="HALNotSelected";
    var evtidx=0;
    var numid=0;

    function syntaxHighlight(json) {
        if (typeof json !== 'string') {
             json = JSON.stringify(json, undefined, 2);
        }
        json = json.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
        return json.replace(/("(\\u[a-zA-Z0-9]{4}|\\[^u]|[^\\"])*"(\s*:)?|\b(true|false|null)\b|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?)/g, function (match) {
            var cls = 'number';
            if (/^"/.test(match)) {
                if (/:$/.test(match)) {
                    cls = 'key';
                } else {
                    cls = 'string';
                }
            } else if (/true|false/.test(match)) {
                cls = 'boolean';
            } else if (/null/.test(match)) {
                cls = 'null';
            }
            return '<span class="' + cls + '">' + match + '</span>';
        });
    }

    function getParameterByName(name, url) {
        if (!url) {
          url = window.location.href;
        }
        name = name.replace(/[\[\]]/g, "\\$&");
        var regex = new RegExp("[?&]" + name + "(=([^&#]*)|&|#|$)"),
            results = regex.exec(url);
        if (!results) return null;
        if (!results[2]) return '';
        return decodeURIComponent(results[2].replace(/\+/g, " "));
    }

    function replyok(obj) {
            console.log("replyok:" + JSON.stringify(obj));
            $("#output")[0].innerHTML = "OK: "+ syntaxHighlight(obj);
    }

    function replyerr(obj) {
            console.log("replyerr:" + JSON.stringify(obj));
            $("#output")[0].innerHTML = "ERROR: "+ syntaxHighlight(obj);
    }

    function gotevent(obj) {
            console.log("gotevent:" + JSON.stringify(obj));
            $("#outevt")[0].innerHTML = (evtidx++) +": "+JSON.stringify(obj);
    }

    function send(message) {
            var api = $("#api").value;
            var verb = $("#verb").value;
            $("#question")[0].innerHTML = "subscribe: "+api+"/"+verb + " (" + JSON.stringify(message) +")";
            ws.call(api+"/"+verb, {data:message}).then(replyok, replyerr);
    }

     // On button click from HTML page
    function callbinder(api, verb, query) {
            console.log ("api="+api+" verb="+verb+" query=" +query);
            var question = urlws +"/" +api +"/" +verb + "?query=" + JSON.stringify(query);
            $("#question")[0].innerHTML = syntaxHighlight(question);
            ws.call(api+"/"+verb, query).then(replyok, replyerr);
    }

    function init() {

        function onopen() {

        	$("#main")[0].style.visibility = "visible";
            $("#connected")[0].innerHTML = "Binder WS Active";
            $("#connected")[0].style.background  = "lightgreen";
            ws.onevent("*", gotevent);
                
            callbinder('radio', 'subscribe',{value:'frequency'} );
            callbinder('radio', 'subscribe',{value:'station_found'} );
        }

        function onabort() {
        	$("#main")[0].style.visibility = "hidden";
        	$("#connected")[0].innerHTML = "Connected Closed";
        	$("#connected")[0].style.background  = "red";

        	callbinder('radio', 'unsubscribe',{value:'frequency'} );
        	callbinder('radio', 'unsubscribe',{value:'station_found'} );

        }
        ws = new afb.ws(onopen, onabort);
    }
    
    function setband() {
    	band = $("#band input[type='radio']:checked").val();
    	callbinder('radio', 'band', {value : band });
    }

    function checkband() {
    	band = $("#band input[type='radio']:checked").val();
    	callbinder('radio', 'band_supported', { band : band });
    }
    
    function get_frequency_range() {
    	band = $("#band input[type='radio']:checked").val();
    	callbinder('radio', 'frequency_range', { band : band });
    }

    function get_frequency_step() {
    	band = $("#band input[type='radio']:checked").val();
    	callbinder('radio', 'frequency_step', { band : band });
    }

    function get_stereo_mode() {
    	callbinder('radio', 'stereo_mode', { });
    }
        
    function set_stereo_mode() {
    	mode = $("#mode input[type ='radio']:checked").val();
    	callbinder('radio', 'stereo_mode', { value : mode });
    }
    