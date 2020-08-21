color_image[rgb_color(255, 83, 83)] = load_png_image("hak.png", 0, 0)
color_image[rgb_color(95, 255, 83)] = load_png_image("kladivko.png", 0, 0)

readmap("map.png")
C_flip_object(objects[small].obj)
C_start_active_fish(objects[big].obj)
