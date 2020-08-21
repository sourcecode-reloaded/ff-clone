function script_killfish(fish)
   C_setlayerimage(objects[fish].layer, fishes[fish].deadimage)
end

function script_vitalizefish(fish)
   C_setlayerimage(objects[fish].layer, fishes[fish].liveimage)
end

function rgba_color(r, g, b, a)
   return string.char(b, g, r, a)
end

function rgb_color(r, g, b)
   return string.char(b, g, r, 255)
end

fishes = {}
function preparefish(color, type, liveimage, deadimage)
   fishes[color] = {}
   fishes[color].type = type
   fishes[color].liveimage = liveimage
   fishes[color].deadimage = deadimage
end

transparent = rgba_color(0, 0, 0, 0)
solid = rgb_color(0, 0, 0)
small = rgb_color(255, 174, 0)
big = rgb_color(0, 0, 255)

preparefish(small, C_SMALL,
	    C_load_png_image(C_datadir.."/gimages/small.png", 0, 0),
	    C_load_png_image(C_datadir.."/gimages/small_skel.png", 0, 0))
preparefish(big, C_BIG,
	    C_load_png_image(C_datadir.."/gimages/big.png", 0, 0),
	    C_load_png_image(C_datadir.."/gimages/big_skel.png", 0, 0))

color_image = {}
color_laydepth = {}
objects = {}

function readmap(filename)
   local data, width, height = C_openpng(leveldir.."/"..filename)

   C_setroomsize(width, height)

   local x, y
   local colors = {}
   local obj_num = 0

   for y = 0, (height-1) do
      for x = 0, width-1 do
	 local color = string.sub(data, 4*(x+y*width)+1, 4*(x+y*width+1))

	 if color ~= transparent then
	    if colors[color] == nil then
	       obj_num = obj_num+1
	       colors[color] = {}
	       colors[color].index = lastcol
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

   local color, data
   for color, data in pairs(colors) do
      local x, y
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
      local b,g,r,a =
	 string.byte(color, 1), string.byte(color, 2), string.byte(color, 3), string.byte(color, 4)

      local type
      if fishes[color] then type = fishes[color].type
      elseif color == solid then type = C_SOLID
      elseif r+g+b < 255 then type = C_STEEL
      else type = C_LIGHT end

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

   local obj, data
   for obj, data in pairs(objects) do
      local depth;
      if color_laydepth[obj] then depth = color_laydepth[obj]
      else depth = 0 end

      data.layer = C_new_layer(data.obj, depth)

      local image

      if color_image[obj] then
	 image = color_image[obj]
      elseif fishes[obj] then image = fishes[obj].liveimage
      elseif data.type == C_SOLID then
	 image = C_generwall(data.width, data.height, data.squares)
      elseif data.type == C_STEEL then
	 image = C_genersteel(data.width, data.height, data.squares)
      else
	 image = C_genernormal(data.width, data.height, data.squares)
      end
      C_setlayerimage(data.layer, image)
   end
end

function script_open_level(codename)
   leveldir = C_datadir.."/levels/"..codename.."/"
   C_set_codename(codename);
   dofile(leveldir.."/script.lua")
end

function script_open_user_level(filename)
   local ext, basename, codename, b

   leveldir = string.gsub(filename, "^(.*/)[^/]*$", "%1")
   if b == 0 then leveldir = "./" end
   basename = string.gsub(filename, "^.*/([^/]*)$", "%1")

   ext, b = string.gsub(basename, "^.*%.([^%.]*)$", "%1")
   if b == 0 then ext = nil end

   codename = "user_"..string.gsub(basename, "^(.*)%.[^%.]*$", "%1")

   C_set_codename(codename);
   if ext == "lua" then dofile(filename)
   elseif ext == "png" then readmap(basename)
   else error("Unknown file extension of '"..filename.."'")
   end
end

function load_png_image(filename, x, y)
   return C_load_png_image(leveldir..filename, x, y)
end

function script_end_level()
   color_image = {}
   color_laydepth = {}
   objects = {}
end