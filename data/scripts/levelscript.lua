--[[
U skriptu k mistnosti se predpoklada, ze bude nasledujiciho tvaru:

1) Nastavovani obrazku barvam ci tvarum (aby bylo jasne, ze neni treba obrazku generovat)
2) Nacteni PNG mapy: readmap("map.png")
3) Nastaveni otoceni rybam (nebo i objektum viz trapdoor), 
--]]

--[[
Tento skript se zabyva predevsim dekodovanim PNG mapy mistnosti (funkce readmap) a
obecne otevrenim mistnosti (funkce open_level a open_user_level)
Vedle toho jeste obsahuje funkce na zabiti / oziveni ryb (nastaveni prislusneho obrazku)
script_killfish a script_vitalizefish. A nakonec taky funkci script_end_level() ktera pri
ukonceni mistnosti znici vsechny nashromazdene LUA struktury.
--]]

local function print_by_chars(str) -- ladici tisk - vypise string po znacich
   local i

   print("string_begin")
   for i = 1, (string.len(str)) do
      print(string.byte(str, i)..": '"..string.sub(str, i, i).."'")
   end
   print("string_end")
end

--[[
PNG mapa je ziskana jako string, ve kterem jsou naskladane pixely po radcich. Kazdy
pixel je reprezentovany ctyrmi znaky. Po zadani hodnot cervene, zelene, modre a alpha slozky
(v intervalu 0 az 255) prislusnou ctverici znaku vyhodi nasledujici funkce:
--]]
function rgba_color(r, g, b, a)
   if r < 0 or g < 0 or g < 0 or a < 0 or
      r > 255 or g > 255 or b > 255 or a > 255 then
      print("LUA warning: color ("..r..", "..g..", "..b..", "..a..") out of range.")
      r = 0 g = 0 b = 0 a = 0
   end
   return string.char(b, g, r, a)
end

-- alpha je casto 255, takze pro zkraceni tu je jeste funkce
function rgb_color(r, g, b)
   return rgba_color(r, g, b, 255)
end

-- Zakladni barvy
transparent = rgba_color(0, 0, 0, 0) -- pruhledna - prazdne policko
solid = rgb_color(0, 0, 0)           -- cerna - zed
small = rgb_color(255, 174, 0)       -- oranzova - mala ryba
big = rgb_color(0, 0, 255)           -- modra - velka ryba

--[[
Funkce readmap() pouziva barvy transparent a solid k rozpoznavani
prazdnych policek a zdi. Neni tomu tak ovsem u ryb, aby bylo mozne
ze skriptu levelu pridat dalsi ryby.

V poli fishes[] jsou ryby (mala a velka) indexovane svymi barvami.
Kazda ryba ma 3 polozky:
  type (C_SMALL nebo C_BIG),
  liveimage (obrazek zive ryby)
  deadimage (obrazek mrtve ryby)
--]]

fishes = {}

-- funkce, ktera zalozi dalsi rybu v poli fiskes
function preparefish(color, type, liveimage, deadimage)
   fishes[color] = {}
   fishes[color].type = type
   fishes[color].liveimage = liveimage
   fishes[color].deadimage = deadimage
end

preparefish(small, C_SMALL,
	    C_load_png_image(C_datadir.."/gimages/small.png", 0, 0),
	    C_load_png_image(C_datadir.."/gimages/small_skel.png", 0, 0))
preparefish(big, C_BIG,
	    C_load_png_image(C_datadir.."/gimages/big.png", 0, 0),
	    C_load_png_image(C_datadir.."/gimages/big_skel.png", 0, 0))

--[[
Pri rozebirani objektu funkce readmap() defaultne vygeneruje pro kazdy objekt (jiny nez ryba)
obrazek podle jeho typu. Pred tim se ovsem podiva, jestli by se nemel pouzit pro tento objekt
nejaky preddefinovany obrazek. Napred se podiva do pole color_image[c], kde c je barva
(ctyrprvkovy string) objektu v PNG mape. Pokud tam nic neni, podiva se jeste do pole
shape_image[s], kde s je string skladajici se z tvaru objektu (pole nul a jednicek) tesne
nasledovanou sirkou (desitkove soustave).

Podle tvaru jsou ale urcovany pouze lehke objekty (u oceli a zdi ze predpoklada, ze spise budou
mit generovane obrazky).
--]]
color_image = {}
shape_image = {}

--[[
Obdobne jako se urcuji obrazky k objektum, se urcuje i hloubka vrstvy, ve ktere objekt bude.
Defaultne maji objekty vrztvu 0, vyssi cisla znamenaji, ze objekt bude vice vepredu.
--]]
color_laydepth = {}
shape_laydepth = {}

-- nacte obrazek z adresare levelu
function load_png_image(filename, x, y)
   return C_load_png_image(leveldir..filename, x, y)
end

--[[
Nasledujici funkce ocekava string ve tvaru:
XX..
X..X
XXXX
Kde 'X' znaci policko objektu a '.' znaci prazdne policko, sirku pozna
podle prvniho '\n'. Nasledne vrati tvar, kterym je mozne indexovat
pole shape_image a shape_laydepth.
--]]
function get_shape(str)
   local width = string.find(str, "\n")-1
   str = string.gsub(str, "\n", "")
   str = string.gsub(str, "%.", string.char(0))
   str = string.gsub(str, "X", string.char(1))

   return str..width
end

-- Aby bylo prirazeni obrazku tvaru maximalne pohodlne
function shape_attr(str, img_filename, x, y, laydepth)
   local sh = get_shape(str)

   shape_image[sh] = load_png_image(img_filename, x, y)
   shape_laydepth[sh] = laydepth
end

--[[
Prichazi hlavni funkce - readmap, ktera rozebere PNG mapu na objekty. Jako parametr
vyzaduje nazev teto mapy (umisteni bude v adresari levelu).

Objekty nakonec budou ulozeny v poli objects indexovanym barvou. Kazdy prvek
bude mit polozky:
  type (jedna z moznosti C_SMALL, C_BIG, C_LIGHT, C_HEAVY, C_SOLID)
  squares (tvar - pole (string) nul a jednicek)
  width, height (sirka a vyska objektu)
  obj (C-ckovy ukazatel na objekt predavany C-ckovym funkcim)
  layer (C-ckovy ukazatel na vrstvu prislusnou objektu)
--]]
objects = {}

function readmap(filename)
   local data, width, height = C_openpng(leveldir.."/"..filename) -- ziskam string pixelu

   C_setroomsize(width, height) -- a jiz vim, jak velka bude mistnost

   --[[
   Postupne prochazim policka PNG mapy a ukladam pritom informace o jednotlivych barvach:
   Kazda barva obrazku bude mit v poli colors[] udaje:
     left, right (nejmensi a nejvetsi x-ova souradnice, ve ktere je barva zastoupena)
     top, bottom (to same s y-ovou souradnici)
     [x..","..y] (pole, kde je true na souradnicich x, y, ktere maji danou barvu
   --]]
   local colors = {}
   local x, y

   for y = 0, (height-1) do
      for x = 0, (width-1) do
	 local color = string.sub(data, 4*(x+y*width)+1, 4*(x+y*width+1))

	 if color ~= transparent then
	    if colors[color] == nil then
	       colors[color] = {}
	       colors[color].left = x
	       colors[color].right = x
	       colors[color].top = y
	       colors[color].bottom = y
	    else
	       if x < colors[color].left then colors[color].left = x
	       elseif x > colors[color].right then colors[color].right = x
	       end
	       if y < colors[color].top then colors[color].top = y
	       elseif y > colors[color].bottom then colors[color].bottom = y
	       end
	    end
	    colors[color][x..","..y] = true
	 end
      end
   end
   data = nil

   -- Zakladni rozebrani uz je provedeno. Zbyva projit barvy a vyrobit z nich objekty

   local color, data
   for color, data in pairs(colors) do
      -- vyroba tvaru objektu
      local s = ""
      for y = data.top, data.bottom do
	 for x = data.left, data.right do
	    if data[x..","..y] then
	       s = s..string.char(1)
	    else
	       s = s..string.char(0)
	    end
	 end
      end

      local b,g,r,a = -- slozky barvy
	 string.byte(color, 1), string.byte(color, 2), string.byte(color, 3), string.byte(color, 4)

      local type -- urceni typu objektu
      if fishes[color] then type = fishes[color].type
      elseif color == solid then type = C_SOLID
      elseif r+g+b < 255 then type = C_STEEL
      else type = C_LIGHT end

      -- nasazeni ziskanych informaci do pole objects
      objects[color] = {}
      objects[color].type = type
      objects[color].squares = s
      objects[color].width = data.right - data.left + 1
      objects[color].height = data.bottom - data.top + 1
      objects[color].obj =
	 C_new_object(color, type, data.left, data.top,
		      objects[color].width, objects[color].height, s)
   end
   colors = nil

   -- objekty jiz jsou vyrobeny, zbyva k nim jeste dodelat obrazky

   local obj, data
   for obj, data in pairs(objects) do
      local sh -- tvar objektu (ovsem jen, pokud type == C_LIGHT)

      if data.type == C_LIGHT then sh = data.squares .. data.width end

      local depth -- hloubka vrstvy

      if color_laydepth[obj] then depth = color_laydepth[obj]
      elseif sh and shape_laydepth[sh] then depth = shape_laydepth[sh]
      else depth = 0 end

      data.layer = C_new_layer(data.obj, depth)

      local image

      if fishes[obj] then image = fishes[obj].liveimage -- obrazek ryby
      elseif color_image[obj] then image = color_image[obj] -- obrazek prirazeny barve
      elseif sh and shape_image[sh] then image = shape_image[sh] -- obrazek prirazeny tvaru
      elseif data.type == C_SOLID then
	 image = C_generwall(data.width, data.height, data.squares) -- automaticky vygenerovana zed
      elseif data.type == C_STEEL then
	 image = C_genersteel(data.width, data.height, data.squares) -- ocel
      else
	 image = C_genernormal(data.width, data.height, data.squares) -- zluty lehky predmet
      end

      C_setlayerimage(data.layer, image)
   end
end

-- otevre level z menu - v adresari "data/levels/"..codename
function script_open_level(codename)
   leveldir = C_datadir.."/levels/"..codename.."/"
   C_set_codename(codename);
   dofile(leveldir.."/script.lua")
end

-- otevre uzivatelsky level
function script_open_user_level(filename)
   local ext  -- pripona
   local basename -- nazev souboru bez cesty vcetne pripony
   local codename -- "user_"..(basename bez pripony)

   local b -- navratova hodnota gsub, poznam podle toho, zda se nahrazeni zdarilo

   leveldir, b = string.gsub(filename, "^(.*/)[^/]*$", "%1")
   if b == 0 then leveldir = "./" end
   basename = string.gsub(filename, "^.*/([^/]*)$", "%1")

   ext, b = string.gsub(basename, "^.*%.([^%.]*)$", "%1")
   if b == 0 then ext = nil end

   codename = "user_"..string.gsub(basename, "^(.*)%.[^%.]*$", "%1")

   C_set_codename(codename);

   if ext == "lua" then dofile(filename) -- jak mistnost otevrit poznam podle pripony
   elseif ext == "png" then readmap(basename)
   else error("Unknown file extension of '"..filename.."'")
   end
end

function script_killfish(fish) -- priradi rybe obrazek kostricky
   C_setlayerimage(objects[fish].layer, fishes[fish].deadimage)
end

function script_vitalizefish(fish) -- priradi rybe obrazek zive ryby
   C_setlayerimage(objects[fish].layer, fishes[fish].liveimage)
end

-- Co kdyby skript levelu hrabal do fishes? Radsi si je ulozim.
local small_fish = fishes[small]
local big_fish = fishes[big]

-- pri ukonceni levelu vratim vsemy puvodni hodnoty
function script_end_level()
   color_image = {}
   color_laydepth = {}
   shape_image = {}
   shape_laydepth = {}
   objects = {}
   fishes = {}
   fishes[small] = small_fish
   fishes[big] = big_fish
end
