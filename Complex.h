#pragma once
#include <cmath>
#include <string>
#include <sstream>
#include <stdexcept>

struct Complex
{
    double re, im;

    // Constructeur : initialise la partie reelle r et la partie imaginaire i
    Complex(double r = 0.0, double i = 0.0) : re(r), im(i) {}

    // Additionne deux nombres complexes
    Complex operator+(const Complex& o) const { return { re + o.re, im + o.im }; }
    // Soustrait deux nombres complexes
    Complex operator-(const Complex& o) const { return { re - o.re, im - o.im }; }
    // Multiplie deux nombres complexes (formule (a+ib)(c+id) = ac-bd + i(ad+bc))
    Complex operator*(const Complex& o) const
    {
        return { re * o.re - im * o.im, re * o.im + im * o.re };
    }
    // Divise deux nombres complexes ; leve une exception si le diviseur est nul
    Complex operator/(const Complex& o) const
    {
        double d = o.re * o.re + o.im * o.im;
        if (d == 0.0) throw std::domain_error("Division par zero complexe");
        return { (re * o.re + im * o.im) / d, (im * o.re - re * o.im) / d };
    }
    // Additionne et affecte
    Complex& operator+=(const Complex& o) { *this = *this + o; return *this; }
    // Soustrait et affecte
    Complex& operator-=(const Complex& o) { *this = *this - o; return *this; }
    // Multiplie et affecte
    Complex& operator*=(const Complex& o) { *this = *this * o; return *this; }
    // Divise et affecte
    Complex& operator/=(const Complex& o) { *this = *this / o; return *this; }
    // Teste l egalite entre deux nombres complexes
    bool operator==(const Complex& o) const { return re == o.re && im == o.im; }
    // Teste l inegalite entre deux nombres complexes
    bool operator!=(const Complex& o) const { return !(*this == o); }

    // Retourne le conjugue du complexe (partie imaginaire opposee)
    Complex conjugate() const { return { re, -im }; }
    // Retourne le module (norme) du complexe
    double  modulus()   const { return std::sqrt(re * re + im * im); }
    // Retourne l argument (angle en radians) du complexe
    double  argument()  const { return std::atan2(im, re); }

    // Cree un complexe a partir de sa forme polaire (module r, argument theta)
    static Complex fromPolar(double r, double theta)
    {
        return { r * std::cos(theta), r * std::sin(theta) };
    }

    // Retourne une representation textuelle du complexe sous la forme "a + bi"
    std::string toString() const
    {
        std::ostringstream oss;
        oss << re;
        if (im >= 0) oss << " + " << im << "i";
        else         oss << " - " << -im << "i";
        return oss.str();
    }
};
