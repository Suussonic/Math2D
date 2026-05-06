#pragma once
#include "Complex.h"
#include <vector>
#include <cmath>
#include <array>
#include <functional>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// RGBA couleur simple
struct Color4f { float r,g,b,a; };

// Calcule la couleur RGBA d un pixel Mandelbrot selon le nombre d iterations atteint (palette electrique HSV)
inline Color4f mandelbrotColor(int iter, int maxIter)
{
    if (iter == maxIter) return {0,0,0,1};
    float t = (float)iter / maxIter;
    float r = 0.5f + 0.5f * std::cos(6.2831f * (t + 0.0f));
    float g = 0.5f + 0.5f * std::cos(6.2831f * (t + 0.33f));
    float b = 0.5f + 0.5f * std::cos(6.2831f * (t + 0.67f));
    return {r,g,b,1.0f};
}

//  Mandelbrot : renvoie une image (width x height) de couleurs
// Genere une image (width x height) de l ensemble de Mandelbrot avec la fenetre et le nombre d iterations donnes
inline std::vector<Color4f> generateMandelbrot(
    int width, int height, int maxIter,
    double xmin=-2.5, double xmax=1.0,
    double ymin=-1.25, double ymax=1.25)
{
    std::vector<Color4f> img(width * height);
    for (int py = 0; py < height; ++py)
    {
        double cim = ymin + (ymax - ymin) * py / height;
        for (int px = 0; px < width; ++px)
        {
            double cre = xmin + (xmax - xmin) * px / width;
            Complex c(cre, cim), z(0,0);
            int i = 0;
            while (i < maxIter && z.modulus() < 2.0)
            {
                z = z*z + c;
                ++i;
            }
            img[py * width + px] = mandelbrotColor(i, maxIter);
        }
    }
    return img;
}

//  Flocon de Koch (Snowflake) - recursif
// Subdivise recursivement le segment [A,B] selon la construction de Koch et ajoute les points dans pts
inline void kochSegment(std::vector<Complex>& pts,
    const Complex& A, const Complex& B, int depth)
{
    if (depth == 0) { pts.push_back(B); return; }
    Complex d = B - A;
    Complex P1 = A + Complex(1.0/3,0)*d;
    Complex P2 = A + Complex(2.0/3,0)*d;
    Complex tip = P1 + Complex::fromPolar(d.modulus()/3.0, d.argument() + M_PI/3.0);
    kochSegment(pts, A,   P1,  depth-1);
    kochSegment(pts, P1,  tip, depth-1);
    kochSegment(pts, tip, P2,  depth-1);
    kochSegment(pts, P2,  B,   depth-1);
}

// Genere les sommets du flocon de Koch (courbe fractale triangulaire) pour la profondeur et la taille donnees
inline std::vector<Complex> generateSnowflake(int depth, double size=1.0)
{
    double r = size;
    Complex A = Complex::fromPolar(r, -M_PI/2);
    Complex B = Complex::fromPolar(r, -M_PI/2 + 2*M_PI/3);
    Complex C = Complex::fromPolar(r, -M_PI/2 + 4*M_PI/3);

    std::vector<Complex> pts;
    pts.push_back(A);
    kochSegment(pts, A, B, depth);
    kochSegment(pts, B, C, depth);
    kochSegment(pts, C, A, depth);
    return pts;
}

//  Fougere de Barnsley (IFS)
struct IFSTransform {
    double a,b,c,d,e,f;
    double prob;
    Color4f color;
};

// Genere n points du systeme de fonctions iterees (IFS) de la fougere de Barnsley avec leurs couleurs
inline std::vector<std::pair<Complex,Color4f>> generateFern(int n = 100000)
{
    // Fougere de Barnsley
    static const IFSTransform T[] = {
        { 0.00,  0.00,  0.00,  0.16, 0, 0,      0.01f, {0.0f,0.3f,0.0f,1} },
        { 0.85,  0.04, -0.04,  0.85, 0, 1.60f,  0.85f, {0.0f,0.7f,0.0f,1} },
        { 0.20, -0.26,  0.23,  0.22, 0, 1.60f,  0.07f, {0.2f,0.9f,0.2f,1} },
        {-0.15,  0.28,  0.26,  0.24, 0, 0.44f,  0.07f, {0.1f,0.8f,0.1f,1} }
    };

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    std::vector<std::pair<Complex,Color4f>> pts;
    pts.reserve(n);
    double x=0, y=0;
    for (int i=0; i<n; ++i)
    {
        double r = dist(rng);
        int idx = 0;
        double acc = 0;
        for (int k=0; k<4; ++k) {
            acc += T[k].prob;
            if (r < acc) { idx=k; break; }
        }
        const IFSTransform& t = T[idx];
        double nx = t.a*x + t.b*y + t.e;
        double ny = t.c*x + t.d*y + t.f;
        x=nx; y=ny;
        if (i > 20) pts.push_back({ Complex(x,y), t.color });
    }
    return pts;
}

// Genere une foret de plusieurs fougeres de Barnsley translatees horizontalement
inline std::vector<std::pair<Complex,Color4f>> generateForest(int trees=5, int pointsPerTree=50000)
{
    std::vector<std::pair<Complex,Color4f>> all;
    auto base = generateFern(pointsPerTree);
    for (int t=0; t<trees; ++t)
    {
        double tx = (t - trees/2.0) * 3.5;
        for (auto& [p, c] : base)
            all.push_back({ Complex(p.re + tx, p.im), c });
    }
    return all;
}

//  Eclairs
struct Segment { Complex A, B; Color4f color; };

// Construit recursivement un eclair aleatoire entre les points A et B pour la profondeur donnee
inline void lightningRec(std::vector<Segment>& segs,
    Complex A, Complex B, int depth, std::mt19937& rng)
{
    if (depth == 0) {
        float t = 0.6f + 0.4f * (depth % 3) / 2.0f;
        segs.push_back({A, B, {t, t, 1.0f, 1.0f}});
        return;
    }
    std::uniform_real_distribution<double> d(-0.4, 0.4);
    Complex mid = Complex((A.re+B.re)/2 + d(rng)*(B.im-A.im),
                          (A.im+B.im)/2 + d(rng)*(B.re-A.re));
    lightningRec(segs, A, mid, depth-1, rng);
    lightningRec(segs, mid, B, depth-1, rng);
    if (depth >= 2) {
        std::uniform_real_distribution<double> ang(-0.8, 0.8);
        double theta = ang(rng);
        Complex dir = mid - A;
        Complex branch = mid + Complex::fromPolar(dir.modulus()*0.5, dir.argument()+theta);
        lightningRec(segs, mid, branch, depth-2, rng);
    }
}

// Genere un arbre d eclairs aleatoires de profondeur depth avec la graine seed
inline std::vector<Segment> generateLightning(int depth=6, unsigned seed=42)
{
    std::mt19937 rng(seed);
    std::vector<Segment> segs;
    lightningRec(segs, Complex(0, 1), Complex(0, -1), depth, rng);
    return segs;
}

//  Cote de Bretagne
// Genere une cote fractale par deplacement de point median
inline std::vector<Complex> generateBrittanyCoast(int depth=8, unsigned seed=123)
{
    std::mt19937 rng(seed);
    std::vector<Complex> pts = { Complex(-1.0, 0.0), Complex(1.0, 0.0) };

    for (int d=0; d<depth; ++d)
    {
        std::uniform_real_distribution<double> noise(-0.3, 0.3);
        double scale = std::pow(0.65, d);
        std::vector<Complex> next;
        next.push_back(pts[0]);
        for (size_t i=0; i+1<pts.size(); ++i)
        {
            Complex mid = Complex((pts[i].re+pts[i+1].re)/2,
                                  (pts[i].im+pts[i+1].im)/2 + noise(rng)*scale);
            next.push_back(mid);
            next.push_back(pts[i+1]);
        }
        pts = next;
    }
    return pts;
}
