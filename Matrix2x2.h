#pragma once
#include "Complex.h"
#include <stdexcept>

//  Matrice 2x2 et conversion complexe <-> matrice
struct Matrix2x2
{
    double a, b, c, d;

    // Constructeur : initialise la matrice avec ses 4 coefficients
    Matrix2x2(double a=1,double b=0,double c=0,double d=1)
        : a(a), b(b), c(c), d(d) {}

    // Produit matriciel
    Matrix2x2 operator*(const Matrix2x2& o) const
    {
        return { a*o.a + b*o.c,  a*o.b + b*o.d,
                 c*o.a + d*o.c,  c*o.b + d*o.d };
    }

    // Addition
    Matrix2x2 operator+(const Matrix2x2& o) const
    {
        return { a+o.a, b+o.b, c+o.c, d+o.d };
    }

    // Determinant
    double det() const { return a*d - b*c; }

    // Inverse
    Matrix2x2 inverse() const
    {
        double dt = det();
        if (dt == 0.0) throw std::domain_error("Matrice singuliere");
        return { d/dt, -b/dt, -c/dt, a/dt };
    }

    // Teste l egalite entre deux matrices 2x2
    bool operator==(const Matrix2x2& o) const
    {
        return a==o.a && b==o.b && c==o.c && d==o.d;
    }

    // Construit la matrice de representation d un nombre complexe z = x+iy : | x -y | / | y  x |
    static Matrix2x2 fromComplex(const Complex& z)
    {
        return { z.re, -z.im, z.im, z.re };
    }

    // Extrait le nombre complexe encode dans la matrice (suppose etre de la forme complexe)
    Complex toComplex() const
    {
        // Verifie que la matrice est bien de la forme complexe
        return { a, c };   // re = a = d, im = c = -b
    }
};

// Multiplie deux complexes via leur representation matricielle 2x2
inline Complex complexMulViaMatrix(const Complex& z1, const Complex& z2)
{
    Matrix2x2 m1 = Matrix2x2::fromComplex(z1);
    Matrix2x2 m2 = Matrix2x2::fromComplex(z2);
    return (m1 * m2).toComplex();
}
