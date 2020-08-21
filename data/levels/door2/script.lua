color_image[rgb_color(255, 255, 255)]  = load_png_image("dvere1.png", 0,0)
color_image[rgb_color(217, 217, 217)]  = load_png_image("dvere2.png", 0,0)
color_image[rgb_color(166, 166, 166)]  = load_png_image("dvere3.png", 0,0)
color_image[rgb_color( 83, 255, 136)]  = load_png_image("dvere4.png", 0,0)

color_image[rgb_color(136, 144, 255)]  = load_png_image("kamen.png", 0,0)
color_image[rgb_color(252, 255,   0)]  = load_png_image("drahokam2.png", 0,0)
color_image[rgb_color(  0, 255, 255)]  = load_png_image("sasanka.png", 0,0)

readmap("map.png")

C_flip_object(objects[small].obj)
C_flip_object(objects[big].obj)
C_start_active_fish(objects[big].obj)
