#ifndef MATERIAL_H
#define MATERIAL_H

#include "rt.hpp"

#include "../include/tinyxml2.h"

struct hit_record;

class material {
    public:
        virtual bool scatter(
            const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
        ) const = 0;        
        virtual tinyxml2::XMLElement* to_xml(tinyxml2::XMLDocument& xmlDoc) const {return nullptr;};
        static std::shared_ptr<material> material_from_xml(tinyxml2::XMLElement* pElement);
};

class lambertian : public material {
    public:
        lambertian(const color& a) : albedo(a) {}

        lambertian(tinyxml2::XMLElement* pElement) {
            tinyxml2::XMLElement * color = pElement->FirstChildElement("Color");

            albedo = vec3(color->DoubleAttribute("r"), color->DoubleAttribute("g"), color->DoubleAttribute("b"));
        }

        virtual bool scatter(
            const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
        ) const override {
            auto scatter_direction = rec.normal + random_unit_vector();
            
            // Catch degenerate scatter direction
            if (scatter_direction.near_zero())
                scatter_direction = rec.normal;
                
            scattered = ray(rec.p, scatter_direction, r_in.time());
            attenuation = albedo;
            return true;
        }

        tinyxml2::XMLElement* to_xml(tinyxml2::XMLDocument& xmlDoc) const {
            tinyxml2::XMLElement * pElement = xmlDoc.NewElement("Lambertian");

            tinyxml2::XMLElement * color = xmlDoc.NewElement("Color");
            color->SetAttribute("r", albedo.x());
            color->SetAttribute("g", albedo.y());
            color->SetAttribute("b", albedo.z());

            pElement->InsertEndChild(color);

            return pElement;
        }

    public:
        color albedo;
};

class metal : public material {
    public:
        metal(const color& a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}

        metal(tinyxml2::XMLElement* pElement) {
            fuzz = pElement->DoubleAttribute("Fuzz");
            tinyxml2::XMLElement * color = pElement->FirstChildElement("Color");
            albedo = vec3(color->DoubleAttribute("r"), color->DoubleAttribute("g"), color->DoubleAttribute("b"));
        }

        virtual bool scatter(
            const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
        ) const override {
            vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
            scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere(), r_in.time());
            attenuation = albedo;
            return (dot(scattered.direction(), rec.normal) > 0);
        }

        tinyxml2::XMLElement* to_xml(tinyxml2::XMLDocument& xmlDoc) const {
            tinyxml2::XMLElement * pElement = xmlDoc.NewElement("Metal");

            tinyxml2::XMLElement * color = xmlDoc.NewElement("Color");
            color->SetAttribute("r", albedo.x());
            color->SetAttribute("g", albedo.y());
            color->SetAttribute("b", albedo.z());

            pElement->InsertEndChild(color);

            pElement->SetAttribute("Fuzz", fuzz);

            return pElement;
        }

    public:
        color albedo;
        double fuzz;
};

class dielectric : public material {
    public:
        dielectric(double index_of_refraction) : ir(index_of_refraction) {}

        dielectric(tinyxml2::XMLElement* pElement) {
            ir = pElement->DoubleAttribute("Ir");
        }

        virtual bool scatter(
            const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
        ) const override {
            attenuation = color(1.0, 1.0, 1.0);
            double refraction_ratio = rec.front_face ? (1.0/ir) : ir;

            vec3 unit_direction = unit_vector(r_in.direction());
            double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
            double sin_theta = sqrt(1.0 - cos_theta*cos_theta);

            bool cannot_refract = refraction_ratio * sin_theta > 1.0;
            vec3 direction;

            if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
                direction = reflect(unit_direction, rec.normal);
            else
                direction = refract(unit_direction, rec.normal, refraction_ratio);

            scattered = ray(rec.p, direction, r_in.time());
            return true;
        }

        tinyxml2::XMLElement* to_xml(tinyxml2::XMLDocument& xmlDoc) const {
            tinyxml2::XMLElement * pElement = xmlDoc.NewElement("Dielectric");

            pElement->SetAttribute("Ir", ir);

            return pElement;
        }

    public:
        double ir; // Index of Refraction
        
	private:
		static double reflectance(double cosine, double ref_idx) {
			// Use Schlick's approximation for reflectance.
			auto r0 = (1-ref_idx) / (1+ref_idx);
			r0 = r0*r0;
			return r0 + (1-r0)*pow((1 - cosine),5);
		}
};

std::shared_ptr<material> material::material_from_xml(tinyxml2::XMLElement* pElement) {
    tinyxml2::XMLElement* matElement = pElement->FirstChildElement();
    if (strcmp(matElement->Name(), "Lambertian") == 0) {
        return std::make_shared<lambertian>(matElement);
    }
    else if (strcmp(matElement->Name(), "Metal") == 0) {
        return std::make_shared<metal>(matElement);
    }
    else if (strcmp(matElement->Name(), "Dielectric") == 0) {
        return std::make_shared<dielectric>(matElement);
    }
    else {
        throw std::invalid_argument("Material " + std::string(matElement->Name()) + " isn't defined");
    }
} 

#endif
