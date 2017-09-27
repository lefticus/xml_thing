#ifndef CONSTEXPR_RANDOM_HPP
#define CONSTEXPR_RANDOM_HPP

constexpr auto seed() noexcept {
  std::uint64_t shifted = 0;

  for( const auto c : __TIME__ )
  {
    shifted <<= 8;
    shifted |= c;
  }

  return shifted;
}

struct PCG {
  struct pcg32_random_t { std::uint64_t state=0;  std::uint64_t inc=seed(); };
  pcg32_random_t rng;
  typedef std::uint32_t result_type;

  constexpr result_type operator()() noexcept
  {
    return pcg32_random_r();
  }

  static result_type constexpr min() noexcept
  {
    return std::numeric_limits<result_type>::min();
  }

  static result_type constexpr max() noexcept
  {
    return std::numeric_limits<result_type>::min();
  }

  private:
  constexpr std::uint32_t pcg32_random_r() noexcept
  {
    std::uint64_t oldstate = rng.state;
    // Advance internal state
    rng.state = oldstate * 6364136223846793005ULL + (rng.inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    std::uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    std::uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
  }

};

template<typename T, typename Gen>
constexpr auto distribution(Gen &g, T min, T max) noexcept
{
  const auto range = max - min + 1;
  const auto bias_remainder = std::numeric_limits<T>::max() % range;
  const auto unbiased_max = std::numeric_limits<T>::max() - bias_remainder - 1;

  auto r = g();
  for (; r > unbiased_max; r = g());

  return (r % range) + min;
}


#endif

