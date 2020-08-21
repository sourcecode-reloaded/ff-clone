color_image[rgb_color(255, 214, 214)]  = load_png_image("spunt.png", 0,0)
color_image[rgb_color(170, 255, 230)] = load_png_image("skeble.png", 0,0)
color_image[rgb_color(206, 255, 170)] = load_png_image("skeble2.png", 0,0)

readmap("map.png")

C_start_active_fish(objects[small].obj)
