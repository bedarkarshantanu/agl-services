--[[
  Copyright (C) 2016 "IoT.bzh"
  Author Fulup Ar Foll <fulup@iot.bzh>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.


  Provide sample LUA routines to be used with AGL control "lua_docall" API
--]]

--global counter to keep track of calls
_count=0


_interval={
  {"km/h",1}, --the "1" should never really get used but
  {"mi/h",0.62137119223733},
  {"m/s",0.27777777777778},
}

_result = {}
_positions={}
for i=1,3 do
  _positions[_interval[i][1]]=i
end

--function _Unit_Converter(value, sourceunits, targetunits)
function _Unit_Converter(source, args, event)
  local value = event["value"]
  local sourceunits = args["from"]
  local targetunits = args["to"]
  local sourcei, targeti = _positions[sourceunits], _positions[targetunits]
  assert(sourcei and targeti)

  if sourcei<targeti then

    local base=1
    for i=sourcei+1,targeti do
      base=base*_interval[i][2]
    end

    event["value"] = value/base
  elseif sourcei>targeti then

    local base=1
    for i=targeti+1,sourcei do
      base=base*interval[i][2]
    end

    event["value"] = value/base
  else
    print("No conversion")
  end

  print('Converted value is: '.. event['value'] ..  args["to"])

  _lua2c['setSignalValueWrap'](source, event)
end

-- Display receive arguments and echo them to caller
function _Simple_Echo_Args (source, args, event)
    _count=_count+1
    AFB:notice(source, "LUA OnCall Echo Args count=%d args=%s event=%s", count, args, event)

    print ("--inlua-- source=", Dump_Table(source))
    print ("--inlua-- args=", Dump_Table(args))
    print ("--inlua-- event=", Dump_Table(event))
end

local function Test_Async_CB (request, result, context)
   response={
     ["result"]=result,
     ["context"]=context,
   }

   AFB:notice (source, "Test_Async_CB result=%s context=%s", result, context)
   AFB:success (request, response)
end

function _Test_Call_Async (request, args)
   local context={
     ["value1"]="abcd",
     ["value2"]=1234
   }

   AFB:notice (source, "Test_Call_Async args=%s cb=Test_Async_CB", args)
   AFB:service(source, "monitor","ping", "Test_Async_CB", context)
end

function _Simple_Monitor_Call (request, args)

    AFB:notice (source, "_Simple_Server_Call args=%s", args)
    local err, result= AFB:servsync (source, "monitor","get", args)
    if (err) then
        AFB:fail (source, "AFB:service_call_sync fail");
    else
        AFB:success(source, request, result["response"])
    end
end

