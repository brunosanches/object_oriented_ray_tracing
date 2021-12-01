#include "engine.hpp"
#include "color.hpp"
#include "vec3.hpp"
#include "ray.hpp"
#include "hittable_list.hpp"
#include "sphere.hpp"
#include "rt.hpp"
#include "camera.hpp"
#include "material.hpp"
#include <iostream>

Engine::Engine(sf::Texture& texture, int img_width, int img_height) :
    texture(texture), img_width(img_width), img_height(img_height), pixels(img_width*img_height*4) {}


// void Engine::createImage() {
//     for(auto i = 0; i < img_height*img_width*4; i+=4) {

//         // Get coordinates based on i
//         // lin must begins from img_height -1 to 0 in order to print the image
//         // in the correct rotation
//         // This function is just to test
//         int col = (i/4) % (img_width);
//         int lin =(img_height-1) - ((i / 4) / img_width);
//         auto r = double(col) / (img_width-1);
//         auto g = double(lin) / (img_height-1);
//         auto b = 0.25;
//         auto a = 1.0;

//         // if(i == img_height*img_width*4 - 4) {
//         //     std::cout << img_height << " " << img_width << std::endl;
//         //     std::cout << ((i / 4) / img_width) << " " << col << std::endl;
//         //     std::cout << r << " " << g << std::endl;
//         // }
//         sf::Uint8 ir = static_cast<sf::Uint8>(255.999 * r);
//         sf::Uint8 ig = static_cast<sf::Uint8>(255.999 * g);
//         sf::Uint8 ib = static_cast<sf::Uint8>(255.999 * b);
//         sf::Uint8 ia = static_cast<sf::Uint8>(255.999 * a);

//         pixels[i] = ir;
//         pixels[i + 1] = ig;
//         pixels[i + 2] = ib;
//         pixels[i + 3] = ia;
//     }
// }

double hit_sphere(const point3& center, double radius, const ray& r) {
    vec3 oc = r.origin() - center;
    auto a = r.direction().length_squared();
    auto half_b = dot(oc, r.direction());
    auto c = oc.length_squared() - radius*radius;
    auto discriminant = half_b*half_b - a*c;
    
    if (discriminant < 0) {
        return -1.0;
    } else {
        return (-half_b - sqrt(discriminant) ) / a;
    }
}

color ray_color(const ray& r, const hittable& world, int depth) {
    hit_record rec;
    
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return color(0,0,0);
        
    if (world.hit(r, 0.001, infinity, rec)) {
        //~ point3 target = rec.p + rec.normal + random_unit_vector();
        // on peut choisir plusieurs méthodes de rendu diffu -> légères différences de rendu
        
        //~ point3 target = rec.p + random_in_hemisphere(rec.normal);
        //~ return 0.5 * ray_color(ray(rec.p, target - rec.p), world, depth-1);
        
        ray scattered;
        color attenuation;
        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            return attenuation * ray_color(scattered, world, depth-1);
        return color(0,0,0);
    }
    vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5*(unit_direction.y() + 1.0);
    return (1.0-t)*color(1.0, 1.0, 1.0) + t*color(0.5, 0.7, 1.0);
}

hittable_list random_scene() {
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return world;
}

void Engine::createImage() 
{
	// Image
    const auto aspect_ratio = 3.0 / 2.0;
    const int image_width = 400;
    
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    
    const int samples_per_pixel = 20;
    const int max_depth = 50; // param à modifier pour aller moins profondément pour la récursivité : 50 de base
    
    // World
    
    //~ auto R = cos(pi/4);
    //~ hittable_list world;

    //~ auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
	//~ auto material_center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
	//~ auto material_left   = make_shared<dielectric>(1.5);
	//~ auto material_right  = make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);

	//~ world.add(make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
	//~ world.add(make_shared<sphere>(point3( 0.0,    0.0, -1.0),   0.5, material_center));
	//~ world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));
	//~ world.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0), -0.45, material_left));
	//~ world.add(make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));
	
	auto world = random_scene();

    // Camera

    //~ camera cam(point3(-2,2,1), point3(0,0,-1), vec3(0,1,0), 20, aspect_ratio);
	point3 lookfrom(13,2,3);
    point3 lookat(0,0,0);
    
	vec3 vup(0,1,0);
	
	auto dist_to_focus = 10.0;
    auto aperture = 0.1;

	camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);
		
	// Render
    if (changed) {
        pixels.clear();
        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
        for (int j = image_height-1; j >= 0; --j) {
            std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0, 0, 0);
                for (int s = 0; s < samples_per_pixel; ++s) {
                    auto u = (i + random_double()) / (image_width-1);
                    auto v = (j + random_double()) / (image_height-1);
                    ray r = cam.get_ray(u, v);
                    pixel_color += ray_color(r, world, max_depth);
                }
                write_color(pixels, pixel_color, samples_per_pixel);
            }
	    }
        changed =false;
    }
	
	
	std::cerr << "\nDone.\n";
}

void Engine::renderImage() {
    createImage();
    texture.create(img_width, img_height);
    texture.update(pixels.data());
    //std::cout << texture.getSize().x << " " << texture.getSize().y << std::endl;
}

void Engine::renderImage(sf::Texture& new_texture) {
    texture = new_texture;
    renderImage();
}

void Engine::renderImage(int new_img_width, int new_img_height) {
    if (img_height != new_img_height || img_width != new_img_width) {
        img_height = new_img_height;
        img_width = new_img_width;

        pixels.resize(img_height*img_width*4);
    }
    
    renderImage();
}

void Engine::renderImage(sf::Texture& new_texture, int new_img_width, int new_img_height) {
    if (img_height != new_img_height || img_width != new_img_width) {
        img_height = new_img_height;
        img_width = new_img_width;

        
        pixels.resize(img_height*img_width*4);
    }
    texture = new_texture;
    
    renderImage();
}