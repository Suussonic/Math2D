#pragma once
#include "Complex.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//  Transformations du plan en coordonnees complexes
//  Un point P = (x,y) est represente par z = x + iy

// Translation : deplace le point z de t (z -> z + t)
inline Complex translation(const Complex& z, const Complex& t)
{
    return z + t;
}

// Homothetie de centre omega et de rapport k : z -> omega + k*(z - omega)
inline Complex homothetie(const Complex& z, const Complex& omega, double k)
{
    return omega + Complex(k, 0) * (z - omega);
}

// Rotation d angle theta autour du centre omega : z -> omega + e^(i*theta)*(z - omega)
inline Complex rotation(const Complex& z, const Complex& omega, double theta)
{
    Complex rot = Complex::fromPolar(1.0, theta);
    return omega + rot * (z - omega);
}

// Similitude directe de parametre a (complexe non nul) et translation b : z -> a*z + b
inline Complex similitude(const Complex& z, const Complex& a, const Complex& b)
{
    return a * z + b;
}

// Application a un polygone
using Polygon = std::vector<Complex>;

// Applique une translation a tous les points d un polygone
inline Polygon applyTranslation(const Polygon& poly, const Complex& t)
{
    Polygon res; res.reserve(poly.size());
    for (auto& p : poly) res.push_back(translation(p, t));
    return res;
}

// Applique une homothetie a tous les points d un polygone
inline Polygon applyHomothetie(const Polygon& poly, const Complex& omega, double k)
{
    Polygon res; res.reserve(poly.size());
    for (auto& p : poly) res.push_back(homothetie(p, omega, k));
    return res;
}

// Applique une rotation a tous les points d un polygone
inline Polygon applyRotation(const Polygon& poly, const Complex& omega, double theta)
{
    Polygon res; res.reserve(poly.size());
    for (auto& p : poly) res.push_back(rotation(p, omega, theta));
    return res;
}

// Applique une similitude a tous les points d un polygone
inline Polygon applySimilitude(const Polygon& poly, const Complex& a, const Complex& b)
{
    Polygon res; res.reserve(poly.size());
    for (auto& p : poly) res.push_back(similitude(p, a, b));
    return res;
}

// Figures predefinies
// Cree un carre centre en (cx, cy) de cote side, retourne ses 4 sommets
inline Polygon makeSquare(double cx, double cy, double side)
{
    double h = side / 2.0;
    return {
        Complex(cx-h, cy-h), Complex(cx+h, cy-h),
        Complex(cx+h, cy+h), Complex(cx-h, cy+h)
    };
}

// Cree un triangle equilateral centre en (cx, cy) inscrit dans un cercle de rayon r
inline Polygon makeTriangle(double cx, double cy, double r)
{
    Polygon p;
    for (int i = 0; i < 3; ++i)
        p.push_back(Complex(cx + r * std::cos(i * 2*M_PI/3 - M_PI/2),
                            cy + r * std::sin(i * 2*M_PI/3 - M_PI/2)));
    return p;
}

