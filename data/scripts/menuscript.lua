---------------------------------------------------------------------
---------------------------   Vyrabeni vetvi   ----------------------

--[[
Pocet mistnosti je v promenne nodesnum, udaje o kazde mistnosti pak
v poli node[], ktere je indexovano codename mistnosti.
Prvky tohoto pole obsahuji polozky:
  x, y (souradnice na mape)
  enabled (mistnost je modra nebo zluta)
  solved (mistnost je jiz vyresena, i kdyz nemusi byt zobrazena pri nekonzistentnim solved.txt)

  parent (codename predchozi mistnosti, virtualni predmistnost pred prvnimi ma codename "")
  enable (seznam mistnosti, ktere se maji aktivovat pri vyreseni, opak k parent
	  reseno jako pole samych true indexovane codename)

  parentshow (mistnost, pri jejiz vyreseni bude dany level zobrazen)
  show (seznam levelu, ktere se maji zobrazit pri vyreseni dane mistnosti,
	opak parentshow)
--]]

nodesnum = 0
node = {}

-- Virtualni predlevel
node[""] = {}
node[""].parentshow = ""
node[""].show = {}
node[""].enable = {}

-- Nasledujici funkce zalozi dalsi mistnost. Parametr hide znaci, ze uvozuje skryvanou vetev.
function menu_node(parent, codename, x, y, hide)
   if node[parent] == nil then error("Missing parent"..parent.."for level "..codename) end

   node[codename] = {}
   node[codename].parent = parent

   if hide then node[codename].parentshow = parent
      -- u zacatku skryvane vetve je parentshow stejny jako parent
   else node[codename].parentshow = node[parent].parentshow
      -- jinak zdedi parentshow po svem rodici
   end

   -- propojeni s parent, parentshow
   node[node[codename].parentshow].show[codename] = true
   node[parent].enable[codename] = true

   -- tato mistnosti zatim zadne deti nema
   node[codename].show = {}
   node[codename].enable = {}

   -- souradnice
   node[codename].x = x
   node[codename].y = y

   nodesnum = nodesnum+1
end

function level_name(codename, name) -- prirazeni ceskeho nazvu
   node[codename].name = name
end

-- Pro jeste vetsi pohodlnost zapisovani vetvi tu mam jeste dve funkce:

-- zalozi novou vetev navazujici na codename, na zacatku je treba zalozit vetev navazujici na ""
function new_branch(codename)
   lastlev = codename
   lastlev_fork = true
end

-- prida mistnost do aktualni vetve
function next_node(codename, x, y)
   menu_node(lastlev, codename, x, y, lastlev_fork)
   lastlev = codename
   lastlev_fork = false
end

-------------------------   Vyrabeni vetvi   ------------------------
---------------------------------------------------------------------
-----------------------   Vyreseni mistnosti   ----------------------

--[[
Aby si hra pamatovala mistnosti vyresene z minula, je seznam
techto mistnosti ukladan do souboru ~/.ff-clone/solved.txt

Na kazdem radku je codename a za nim pak za lomitkem pocet tahu.
V puvodni verzi ale pocet tahu ukladan nebyl, takze podporuji i
format be nej.
--]]

function shownode(codename) -- zobrazeni kolecka (zatim hnedeho)
   local parent = node[codename].parent

   if parent ~= "" then
      C_add_line(node[parent].x, node[parent].y,
		     node[codename].x, node[codename].y)
   end

   node[codename].cnode =
      C_add_node(node[codename].x, node[codename].y, codename)
end

--[[
Pri vyreseni mistnosti se zavola nasledujici funkce. Ta zobrazi mistnosti ze seznamu
show a zmodra mistnosti ze seznamu enabled. Teprve pr tomto zmodrani se mistnostem nacte
nahled a nastavi nazev.
--]]
function enable_children(codename)
   local name, b

   -- zobrazeni skrytych mistnosti kvuli prave vyresene
   for name, b in pairs(node[codename].show) do
      shownode(name)
   end

   -- zmodrani aktivovanych mistnosti
   for name, b in pairs(node[codename].enable) do
      node[name].enabled = true
      C_enable_node(node[name].cnode)
      if node[name].name then update_level_name(node[name], name) end -- nazev
      C_load_level_icon(node[name].cnode, C_datadir.."/levels/"..name.."/icon.png") -- nahled

      if node[name].solved then
	 --[[
	 pri nekonzistentim solved.txt mohla byt vyresena nedosazena mistnost,
	 teprve ted ji uznam za vyresenou
	 --]]
	 C_solved_node(node[name].cnode)
	 enable_children(name)
      end
   end
end

solvednum = 0 -- pocet vyresenych mistnosti
solvedlist = {} -- pole (indexovane cisly) vyresenych mistnosti

local loadmode  -- mod nacitani souboru solved.txt, v tomto modu napriklad nic neukladam

local firstsee = true -- je funkce see_hf_moves volana poprve? (pri tomto behu programu)

-- zobrazi okno, ze by se mel hrac podivat do sine slavy, pokud ma nejkratsi reseni
local function see_hf_moves(codename, moves, omoves)
   if firstsee then
      print("see http://www.olsak.net/ff-clone/halloffame_en.html (level "..codename..")")
      firstsee = false
   end

   if not hf_moves or loadmode then return end

   local bestmov = hf_moves[codename] -- pocet tahu v sini slavy

   if not bestmov then return end
   if moves >= bestmov then return end -- neprekonal sin slavy
   if omoves and omoves < bestmov then return end -- prekonal ji jiz drive
   C_infowindow([[
Congratulations!

Your solution of the level ']]..codename..[[' (]]..moves..[[)
is better than the best known (]]..bestmov..[[).

See Hall of Fame:
http://www.olsak.net/ff-clone/halloffame_en.html

             Click to continue]])
end

-- zobrazi okno, ze by se mel hrac podivat do sine slavy (protoze vyresil hru)
local function see_hf_solved()
   if toomanysolvers then return end

   C_infowindow([[
Congratulations!

You have solved this game!

See Hall of Fame:
http://www.olsak.net/ff-clone/halloffame_en.html

             Click to continue]])
end

-- Nacte soubor solved.txt s vyresenymi mistnostmi.
function solved_load()
   loadmode = true

   if C_homedir then
      local solved_file = io.open(C_homedir.."solved.txt", "r")
      if solved_file then
	 local line = solved_file:read("*l")
	 while line do
	    local codename, moves, b
	    codename = string.gsub(line, "^(.*)/%d*$", "%1") -- codename
	    moves, b = string.gsub(line, "^.*/(%d*)$", "%1") -- pocet tahu
	    if b == 0 then moves = nil -- pocet tahu tam nebyl
	    else moves = tonumber(moves) end

	    script_solved_node(codename, moves) -- emuluje vyreseni mistnosti
	    line = solved_file:read("*l") -- prechod na dalsi radek
	 end
	 solved_file:close()
      end
   end

   loadmode = false
end

-- Ulozi soubor solved.txt. Seznam vyresenych mistnosti bere z pole solved.
function solved_save()
   if loadmode or not C_homedir then return end
   local solved_file = io.open(C_homedir.."solved.txt", "w")
   if not solved_file then return end

   local i
   for i = 0, (solvednum-1) do
      solved_file:write(solvedlist[i])
      if node[solvedlist[i]].moves then
	 solved_file:write("/", node[solvedlist[i]].moves) end
      solved_file:write("\n")
   end
   solved_file:close()
end

-- Zaregistruje novou vyresenou mistnost
function solved_savenew(codename)
   solvedlist[solvednum] = codename -- prida do seznamu
   solvednum = solvednum + 1

   if loadmode or not C_homedir then return end

   -- prida do souboru solved.txt
   local solved_file = io.open(C_homedir.."solved.txt", "a+")
   if not solved_file then return end

   solved_file:write(codename, "/", node[codename].moves, "\n")
   solved_file:close()

   if solvednum == nodesnum then see_hf_solved() end
end

-- Nastavi nazev mistnosti zobrazovany pri najeti mysi (tedy vcetne poctu tahu)
function update_level_name(level, codename)
   if not level.name or not level.enabled then return end
   local title = level.name

   if level.moves then -- pridavam pocet tahu
      title = title.." "..level.moves
      if hf_moves and hf_moves[codename] then -- pocet tahu v sini slavy
	 title = title.."/"..hf_moves[codename]
	 if(level.moves < hf_moves[codename]) then
	    title = title.."(!)" -- hrac prekonal sin slavy
	 end
      end
   end

   C_set_level_name(level.cnode, title) -- predani nazvu C-cku
end

--[[
Funkce volana pri vyreseni mistnosti $codename za $moves tahu.
--]]
function script_solved_node(codename, moves)
   local level = node[codename]

   if not level then
      print("LUA warning: unknown level '"..codename.."'")
      return
   end

   if not level.solved then -- nove vyresena mistnost
      if level.enabled then
	 C_solved_node(node[codename].cnode)
	 enable_children(codename)
      end
      if moves then
	 see_hf_moves(codename, moves)
	 level.moves = moves
	 update_level_name(level, codename)
      end
      solved_savenew(codename)
      level.solved = true
   elseif moves and (not level.moves or moves < level.moves) then
      -- mistnost uz byla vyresena, ted se jenom vylepsil pocet tahu
      see_hf_moves(codename, moves, level.moves)
      level.moves = moves
      update_level_name(level, codename)
      solved_save()
   end
end

-----------------------   Vyreseni mistnosti   ----------------------
---------------------------------------------------------------------
-----------------------   Nacteni mistnosti   -----------------------

dofile(C_datadir.."scripts/menumap.lua")    -- rozmisteni kolecek
dofile(C_datadir.."scripts/levelnames.lua") -- ceske nazvy mistnosti
dofile(C_datadir.."scripts/halloffame.lua") -- nejmensi zname pocty tahu

enable_children("") -- vyreseni virtualni predmistnosti -> zobrazeni a zmodrani spravnych kolecek
solved_load() -- nacteni seznamu vyresenych mistnosti
