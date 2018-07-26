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


  Provide Sample Timer Handing to push event from LUA
--]]

-- Create event on Lua script load
_MyContext={}

-- WARNING: call back are global and should start with '_'
function _Timer_Test_CB (timer, context)

   local evtinfo= AFB:timerget(source, timer)
   printf ("[-- _Timer_Test_C --] evtInfo=%s", Dump_Table(evtinfo))

   --send an event an event with count as value
   AFB:evtpush (source, _MyContext["event"], {["label"]= evtinfo["label"], ["count"]=evtinfo["count"], ["info"]=context["info"]})

   -- note when timerCB return!=0 timer is kill
   return 0

end

-- sendback event depending on count and delay
function _Simple_Timer_Test (request, client)

    local context = {
        ["info"]="My 1st private Event",
    }

    -- if event does not exit create it now.
    if (_MyContext["event"] == nil) then
      _MyContext["event"]= AFB:evtmake(source, client["label"])
    end

    -- if delay not defined default is 5s
    if (client["delay"]==nil) then client["delay"]=5000 end

    -- if count is not defined default is 10
    if (client["count"]==nil) then client["count"]=10 end

    -- we could use directly client but it is a sample
    local myTimer = {
       ["label"]=client["label"],
       ["delay"]=client["delay"],
       ["count"]=client["count"],
    }
    AFB:notice (source, "Test_Timer myTimer=%s", myTimer)

    -- subscribe to event
    AFB:subscribe (source, request, _MyContext["event"])

    -- settimer take a table with delay+count as input (count==0 means infinite)
    AFB:timerset (source, myTimer, "_Timer_Test_CB", context)

    -- nothing special to return send back
    AFB:success (source, request, myTimer)

    return 0
end
