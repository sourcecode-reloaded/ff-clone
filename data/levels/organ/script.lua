color_image[rgb_color(172, 152, 152)] = load_png_image("varhany.png",   0,   -0.06)
color_image[rgb_color(231, 165, 165)] = load_png_image("pistala2.png", -0.02,-0.02)
color_image[rgb_color(176, 231, 165)] = load_png_image("pistala3.png", -0.02,-0.06)
color_image[rgb_color(162, 172, 152)] = load_png_image("pistala4.png", -0.04,-0.02)

readmap("map.png")

C_flip_object(objects[big].obj)
C_flip_object(objects[small].obj)
C_start_active_fish(objects[big].obj)
