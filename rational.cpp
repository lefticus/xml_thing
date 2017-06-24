#include <numeric>
#include <cstdint>
#include <cmath>
#include <tuple>

// adapted from: https://rosettacode.org/wiki/Convert_decimal_number_to_rational#C

template<typename T>
constexpr auto floor(T t)
{
    return static_cast<T>(static_cast<std::int64_t>(t));
}

template<typename F>
constexpr auto rat_approx(F f, int64_t md)
{
    if constexpr (std::is_integral_v<F>) {
        return std::tuple{f, 1};
    } else {
        if (md <= 1) { 
            return std::tuple{static_cast<std::int64_t>(f), static_cast<std::int64_t>(1) }; 
        }
    
        const bool neg = [&](){
            if (f<0) { 
                f = -f;
                return true;
            } else {
                return false;
            }
        }();
    
        std::int64_t n = 1;    
        while (f != floor(f)) { n <<= 1; f *= 2; }
        std::int64_t d = f;
    
        std::int64_t h[3] = { 0, 1, 0 };
        std::int64_t k[3] = { 1, 0, 0 };

        /* continued fraction and check denominator each step */
        for (int i = 0; i < 64; ++i) {
            /*  a: continued fraction coefficients. */
            const auto a = n ? d / n : 0;
            if (i && !a) break;
    
            auto x = d; d = n; n = x % n;
    
            x = a;
            if (k[1] * a + k[0] >= md) {
                x = (md - k[0]) / k[1];
                if (x * 2 >= a || k[1] >= md)
                    i = 65;
                else
                    break;
            }
    
            h[2] = x * h[1] + h[0]; h[0] = h[1]; h[1] = h[2];
            k[2] = x * k[1] + k[0]; k[0] = k[1]; k[1] = k[2];
        }
        return std::tuple{neg ? -h[1] : h[1], k[1]};
    }
}





template<typename Numerator, typename Denominator>
struct Rational
{
  Numerator numerator = 0;
  Denominator denominator = 1;

  template<typename RhsNumerator, typename RhsDenominator>
  auto operator*(const Rational<RhsNumerator, RhsDenominator> &rhs) const {
    return Rational{numerator * rhs.numerator, denominator * rhs.denominator}.simplify();
  }

  template<typename RhsNumerator, typename RhsDenominator>
  auto operator/(const Rational<RhsNumerator, RhsDenominator> &rhs) const {
    return *this * Rational{rhs.denominator, rhs.numerator};
  }

  template<typename RhsNumerator, typename RhsDenominator>
  auto operator-(const Rational<RhsNumerator, RhsDenominator> &rhs) const {
    return *this + Rational{-rhs.numerator, rhs.denominator};
  }

  template<typename RhsNumerator, typename RhsDenominator>
  constexpr auto operator+(const Rational<RhsNumerator, RhsDenominator> &rhs) const {
    const auto lcm = std::lcm(denominator, rhs.denominator);
    const auto lhs_mult = lcm / denominator;
    const auto rhs_mult = lcm / rhs.denominator;
    return Rational{numerator * lhs_mult + rhs.numerator * rhs_mult, lcm}.simplify();
  }

  constexpr auto simplify() const
  {
    const auto gcd = std::gcd(numerator, denominator);
    const auto simplified = Rational{numerator / gcd, denominator / gcd};
    if (numerator < 0 && denominator < 0) {
      return Rational{simplified.numerator * -1, simplified.denominator * -1};
    } else {
      return simplified;
    }
  }
};

template<typename Numerator, typename Denominator>
Rational(Numerator numerator, Denominator denominator)->Rational<Numerator, Denominator>;




template<typename Real, typename Imaginary>
struct Complex
{
  Real real;
  Imaginary imaginary;

  template<typename RhsReal, typename RhsImaginary>
  constexpr Complex operator-(const Complex<RhsReal, RhsImaginary> &rhs) const noexcept
  {
    return Complex{real - rhs.real, imaginary - rhs.imaginary};
  }

  template<typename RhsReal, typename RhsImaginary>
  constexpr Complex operator+(const Complex<RhsReal, RhsImaginary> &rhs) const noexcept
  {
    return Complex{real + rhs.real, imaginary + rhs.imaginary};
  }

  template<typename RhsReal, typename RhsImaginary>
  constexpr Complex operator*(const Complex<RhsReal, RhsImaginary> &rhs) const noexcept
  {
    return Complex{real * rhs.real - imaginary * rhs.imaginary,
                   imaginary * rhs.real + real * rhs.imaginary};
  }

  template<typename RHS>
  constexpr Complex operator*(const RHS &rhs) const noexcept
  {
    return Complex{real * rhs, imaginary * rhs};
  }
};

template<typename Real, typename Imaginary>
Complex(Real real, Imaginary imaginary)->Complex<Real, Imaginary>;



int main()
{
  auto c = Complex{1,3} * Complex{3,4} + Complex{2.9, 3.4} - Complex{1.3, -2.4};
  return c.real;
}

int main(const int argc, const char *[])
{
  auto result = Rational{1, 2} + Rational{1, 3} + Rational{1,4} + Rational{1,5} + Rational{1,6} + Rational{argc,7};
  return result.denominator;
}


