node = {}

node[""] = {}
node[""].parentshow = ""
node[""].show = {}
node[""].enable = {}

function menu_node(parent, codename, x, y, hide)
   if node[parent] == nil then error("Missing parent"..parent) end

   node[codename] = {}
   node[codename].parent = parent

   if hide then node[codename].parentshow = parent
   else node[codename].parentshow = node[parent].parentshow
   end

   node[node[codename].parentshow].show[codename] = true
   node[parent].enable[codename] = true
   node[codename].show = {}
   node[codename].enable = {}

   node[codename].x = x
   node[codename].y = y
end

function level_name(codename, name)
   node[codename].name = name
end

function shownode(codename)
   local parent = node[codename].parent

   if parent ~= "" then
      C_add_line(node[parent].x, node[parent].y,
		     node[codename].x, node[codename].y)
   end

   node[codename].cnode =
      C_add_node(node[codename].x, node[codename].y, codename)
end

function enable_children(codename)
   local name, b
   for name, b in pairs(node[codename].show) do
      shownode(name)
   end
   for name, b in pairs(node[codename].enable) do
      C_enable_node(node[name].cnode)
      if node[name].name then
	 C_set_level_name(node[name].cnode, node[name].name)
      end
      C_load_level_icon(node[name].cnode, C_datadir.."/levels/"..name.."/icon.png")
   end
end

function script_solved_node(codename)
   if C_homedir then
      local solved_file = io.open(C_homedir.."solved.txt", "a+")
      if solved_file then
	 solved_file:write(codename, "\n")
	 solved_file:close()
      end
   end

   C_solved_node(node[codename].cnode)
   enable_children(codename)
end


function new_branch(codename)
   lastlev = codename
   lastlev_fork = true
end

function next_node(codename, x, y)
   menu_node(lastlev, codename, x, y, lastlev_fork)
--   menu_node("", codename, x, y)
   lastlev = codename
   lastlev_fork = false
end

dofile(C_datadir.."scripts/menu.lua")
dofile(C_datadir.."scripts/levelnames.lua")

enable_children("")

if C_homedir then
   local solved_file = io.open(C_homedir.."solved.txt", "r")
   if solved_file then
      local line = solved_file:read("*l")
      while line do
	 C_solved_node(node[line].cnode)
	 enable_children(line)
	 line = solved_file:read("*l")
      end
      io.close(solved_file)
   end
end
