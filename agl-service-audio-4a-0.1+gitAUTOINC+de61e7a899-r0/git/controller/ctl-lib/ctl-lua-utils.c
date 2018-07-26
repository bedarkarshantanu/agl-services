/*
 * Copyright (C) 2016 \"IoT.bzh\"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
  Licensed under the Apache License, Version 2.0 (the \"License\");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an \"AS IS\" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ref:
 *  (manual) https://www.lua.org/manual/5.3/manual.html
 *  (lua->C) http://www.troubleshooters.com/codecorn/lua/lua_c_calls_lua.htm#_Anatomy_of_a_Lua_Call
 *  (lua/C Var) http://acamara.es/blog/2012/08/passing-variables-from-lua-5-2-to-c-and-vice-versa/
 *  (Lua/C Lib)https://john.nachtimwald.com/2014/07/12/wrapping-a-c-library-in-lua/
 *  (Lua/C Table) https://gist.github.com/SONIC3D/10388137
 *  (Lua/C Nested table) https://stackoverflow.com/questions/45699144/lua-nested-table-from-lua-to-c
 *  (Lua/C Wrapper) https://stackoverflow.com/questions/45699950/lua-passing-handle-to-function-created-with-newlib
 *
 */

const char *lua_utils = "--===================================================\n\
--=  Niklas Frykholm\n\
-- basically if user tries to create global variable\n\
-- the system will not let them!!\n\
-- call GLOBAL_lock(_G)\n\
-- \n\
--===================================================\n\
function GLOBAL_lock(t)\n\
  local mt = getmetatable(t) or {}\n\
  mt.__newindex = lock_new_index\n\
  setmetatable(t, mt)\n\
end\n\
\n\
--===================================================\n\
-- call GLOBAL_unlock(_G)\n\
-- to change things back to normal.\n\
--===================================================\n\
function GLOBAL_unlock(t)\n\
  local mt = getmetatable(t) or {}\n\
  mt.__newindex = unlock_new_index\n\
  setmetatable(t, mt)\n\
end\n\
\n\
function lock_new_index(t, k, v)\n\
  if (string.sub(k,1,1) ~= \"_\") then\n\
    GLOBAL_unlock(_G)\n\
    error(\"GLOBALS are locked -- \" .. k ..\n\
          \" must be declared local or prefix with '_' for globals.\", 2)\n\
  else\n\
    rawset(t, k, v)\n\
  end\n\
end\n\
\n\
function unlock_new_index(t, k, v)\n\
  rawset(t, k, v)\n\
end\n\
\n\
-- return serialised version of printable table\n\
function Dump_Table(o)\n\
   if type(o) == 'table' then\n\
      local s = '{ '\n\
      for k,v in pairs(o) do\n\
         if type(k) ~= 'number' then k = '\"'..k..'\"' end\n\
         s = s .. '['..k..'] = ' .. Dump_Table(v) .. ','\
      end\n\
      return s .. '} '\n\
   else\n\
      return tostring(o)\n\
   end\n\
end\n\
\n\
-- simulate C prinf function\n\
printf = function(s,...)\n\
    io.write(s:format(...))\n\
    io.write(\"\")\n\
    return\n\
end\n\
\n\
function sleep(n)\n\
    os.execute(\"sleep \" .. tonumber(n))\n\
end\n\
\n\
function table_eq(table1, table2)\n\
	local avoid_loops = {}\n\
	local function recurse(t1, t2)\n\
	-- compare value types\n\
	if type(t1) ~= type(t2) then return false end\n\
	-- Base case: compare simple values\n\
	if type(t1) ~= \"table\" then return t1 == t2 end\n\
	-- Now, on to tables.\n\
	-- First, let's avoid looping forever.\n\
	if avoid_loops[t1] then return avoid_loops[t1] == t2 end\n\
	avoid_loops[t1] = t2\n\
	-- Copy keys from t2\n\
	local t2keys = {}\n\
	local t2tablekeys = {}\n\
	for k, _ in pairs(t2) do\n\
	if type(k) == \"table\" then table.insert(t2tablekeys, k) end\n\
	t2keys[k] = true\n\
	end\n\
	-- Let's iterate keys from t1\n\
	for k1, v1 in pairs(t1) do\n\
	local v2 = t2[k1]\n\
	if type(k1) == \"table\" then\n\
		-- if key is a table, we need to find an equivalent one.\n\
		local ok = false\n\
		for i, tk in ipairs(t2tablekeys) do\n\
		if table_eq(k1, tk) and recurse(v1, t2[tk]) then\n\
		table.remove(t2tablekeys, i)\n\
		t2keys[tk] = nil\n\
		ok = true\n\
		break\n\
		end\n\
		end\n\
		if not ok then return false end\n\
	else\n\
		-- t1 has a key which t2 doesn't have, fail.\n\
		if v2 == nil then return false end\n\
		t2keys[k1] = nil\n\
		if not recurse(v1, v2) then return false end\n\
	end\n\
	end\n\
	-- if t2 has a key which t1 doesn't have, fail.\n\
	if next(t2keys) then return false end\n\
	return true\n\
	end\n\
	return recurse(table1, table2)\n\
end\n\
-- lock global variable\n\
GLOBAL_lock(_G)\n\
";
