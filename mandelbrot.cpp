#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <iostream>
#include <thread>
#include <future>
#include <complex>
#include <cassert>

template<typename T>
struct Point {
  T x{};
  T y{};
};

template<typename T>
Point(T x, T y) -> Point<T>;


template<typename T>
struct SizeWithStepping {
  T width{};
  T height{};
  T xstepping{1};
  T ystepping{1};
};

template<typename T>
struct Size {
  T width{};
  T height{};
  constexpr SizeWithStepping<T> iterable(const T xstep, const T ystep) const noexcept
  {
    return SizeWithStepping<T>{width, height, xstep, ystep};
  }
};

template<typename T>
struct SizeIterator {
  const T width = 0;
  const T height = 0;
  std::pair<T, T> stepping{1,1};
  std::pair<T, T> loc{0,0};

  constexpr SizeIterator &operator++() noexcept {
    loc.first += stepping.first;
    if (loc.first >= width) {
      loc.first = 0; 
      loc.second += stepping.second;
    }
    return *this;
  }

  constexpr bool operator!=(const SizeIterator<T> &other) const noexcept {
    return loc != other.loc;
  }

  constexpr const std::pair<T, T> &operator*() const noexcept {
    return loc;
  }
  constexpr std::pair<T, T> &operator*() noexcept {
    return loc;
  }
};

template<typename T>
constexpr SizeIterator<T> begin(const SizeWithStepping<T> &t_s) noexcept {
  return SizeIterator<T>{t_s.width, t_s.height, {t_s.xstepping, t_s.ystepping}};
}

template<typename T>
constexpr SizeIterator<T> end(const SizeWithStepping<T> &t_s) noexcept {
  return SizeIterator<T>{t_s.width, t_s.height, {t_s.xstepping, t_s.ystepping}, {0, t_s.height}};
}


template<typename T>
Size(T width, T height) -> Size<T>;

template<typename T>
struct Color {
  T r{};
  T g{};
  T b{};
};

template<typename T>
Color(T r, T g, T b) -> Color<T>;

template<std::size_t Power, typename Value>
constexpr auto pow(Value t_val)
{
  auto result = t_val;
  for(std::size_t itr = 1; itr < Power; ++itr)
  {
    result *= t_val;
  }
  return result;
}

template<typename ComplexType, typename PowerType>
constexpr auto opt_pow(const std::complex<ComplexType> &t_val, PowerType t_power)
{
  if (t_power == static_cast<PowerType>(1.0)) {
    return t_val;
  } else if (t_power == static_cast<PowerType>(2.0)) {
    return std::complex{ pow<2>(std::real(t_val)) - pow<2>(std::imag(t_val)), 2 * std::real(t_val) * std::imag(t_val) };
  } else if (t_power == static_cast<decltype(t_power)>(3.0)) {
    const auto a = std::real(t_val);
    const auto b = std::imag(t_val);
    return std::complex{ -3 * a * pow<2>(b) + pow<3>(a), 3 * pow<2>(a) * b - pow<3>(b) };
//  } else if (t_power == static_cast<decltype(t_power)>(4.0)) {
//    const auto a = std::real(t_val);
//    const auto b = std::imag(t_val);
//    return std::complex{ pow<4>(a) + pow<4>(b) - 6 * pow<2>(a) * pow<2>(b), 4 * pow<3>(a) * b - 4 * a * pow<3>(b) };
  } else {
    return std::pow(t_val, t_power);
  }

}

template<typename PointType, typename CenterType, typename SizeType, typename ScaleType>
auto get_color(const Point<PointType> &t_point, const Point<CenterType> &t_center, 
               const Size<SizeType> &t_size, const ScaleType t_scale, std::size_t max_iteration,
               const CenterType power, const bool do_abs) noexcept
{
  const std::complex scaled {
    t_point.x/(t_size.width/t_scale) + (t_center.x - (t_scale/static_cast<CenterType>(2.0) )),
    t_point.y/(t_size.height/t_scale) + (t_center.y - (t_scale/static_cast<CenterType>(2.0) ))
  };

  auto current = scaled;

  auto iteration = 0u;
  auto stop_iteration = max_iteration;

  while ( iteration < stop_iteration )
  {
    if (std::norm(current) > (2.0*2.0) && stop_iteration == max_iteration) {
      stop_iteration = iteration + 5;
    }

    if (do_abs) {
      current = std::complex{ std::abs(std::real(current)), std::abs(std::imag(current)) };
    }

    current = opt_pow(current, power);
    current += scaled;


    ++iteration;
  }

  if (iteration == max_iteration)
  {
    return Color{0.0,0.0,0.0};
  } else {
    const auto value = ((iteration + 1) - (std::log(std::log(std::abs(std::real(current) * std::imag(current)))))/std::log(power)); 
    const auto colorval = std::abs(static_cast<int>(std::floor(value * 10.0)));

    const auto colorband = colorval % (256 * 7) / 256;
    const auto mod256 = colorval % 256;
    const auto to_1 = mod256 / 255.0;
    const auto to_0 = 1.0 - to_1;

    switch(colorband) {
      case 0:
        return Color{to_1, 0.0, 0.0};
      case 1:
        return Color{1.0, to_1, 0.0};
      case 2:
        return Color{to_0, 1.0, 0.0};
      case 3:
        return Color{0.0,  1.0, to_1};
      case 4:
        return Color{0.0, to_0, 1.0};
      case 5:
        return Color{to_1, 0.0, 1.0};
      case 6:
        return Color{to_0, 0.0, to_0};
      default:
        std::cout << "colorband: " << colorband << '\n';
        assert(colorband <= 6 && colorband >= 0);
    }
  }
}


struct SetPixel {
  constexpr SetPixel(sf::Image &t_img) noexcept
    : img(t_img)
    {
    }

  sf::Image &img;

  template<typename PointType, typename ColorType>
  void set_pixel(const Point<PointType> &t_point, const Color<ColorType> &t_color)
  {
    const auto to_sf_color = [](const auto &color) {
      const auto to_8bit = [](const auto &f) {
        return static_cast<std::uint8_t>(std::floor(f * 255));
      };
      return sf::Color(to_8bit(color.r), to_8bit(color.g), to_8bit(color.b));
    };
    img.setPixel(t_point.x, t_point.y, to_sf_color(t_color));
  }
};

template<std::size_t BlockWidth, std::size_t BlockHeight>
struct ImageBlock
{
  using Row = std::array<Color<double>, BlockWidth>;

  template<typename T>
  explicit constexpr ImageBlock(const Point<T> p) noexcept 
    : upper_left{p.x, p.y}
  {}

  Point<std::size_t> upper_left;
  std::array<Row, BlockHeight> image;

  static constexpr const auto size = Size<std::size_t>{BlockWidth, BlockHeight};
};

template<size_t BlockWidth, size_t BlockHeight, typename SizeType, typename CenterType, typename ScaleType, typename Container>
void future_pixels(const Size<SizeType> &t_size, const Point<CenterType> &t_center,
                   const ScaleType t_scale, Container &t_container, size_t t_max_iteration, const CenterType t_power, const bool t_do_abs)
{
  t_container.clear();

  for (const auto [x, y] : t_size.iterable(BlockWidth, BlockHeight)) {
    t_container.push_back(
        std::async(
          [p = Point{x, y}, t_size, t_center, t_scale, t_max_iteration, t_power, t_do_abs](){
            using Row = std::array<Color<double>, BlockWidth>;
            ImageBlock<BlockWidth, BlockHeight> image{p};

            for (const auto [x, y] : image.size.iterable(1,1)) {
              image.image[y][x] = get_color(Point{p.x + x, p.y + y}, t_center, t_size, t_scale, t_max_iteration, t_power, t_do_abs);
            }
            return image;
          }
        )
      );
  }
}

template<typename Container>
std::pair<bool, bool> cull_pixels(Container &t_container, SetPixel &t_set_pixel)
{
  const auto done = std::remove_if(t_container.begin(), t_container.end(), 
      [&](auto &f) {
        if (f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
          auto result = f.get();

          for (const auto &[x, y] : result.size.iterable(1,1)) {
            t_set_pixel.set_pixel(
                Point{result.upper_left.x + x, result.upper_left.y + y},
                result.image[y][x]
              );
          }
          return true;
        } else {
          return false;
        }
      }
    );

  const auto did_something = done != t_container.end();

  t_container.erase(done, t_container.end());

  return std::pair{did_something, t_container.empty()};
}

int main() {
  const Size size{640u, 640u};
  constexpr const Size block_size{20u, 20u};

  sf::RenderWindow window(sf::VideoMode(size.width, size.height), "Tilemap");
  window.setVerticalSyncEnabled(true);

  sf::Image img;
  img.create(size.width, size.height);
  sf::Texture texture;
  sf::Sprite bufferSprite(texture);
  SetPixel sp(img);
  texture.loadFromImage(img);

  bufferSprite.setTexture(texture);
  bufferSprite.setTextureRect(sf::IntRect(0,0,size.width,size.height));
  bufferSprite.setPosition(0,0);

  Point center{0.001643721971153, -0.822467633298876};
  auto scale = 3.0;
  auto power = 2.0;
  auto do_abs = false;

  std::vector<std::future<ImageBlock<block_size.width, block_size.height>>> pixels;

  auto see_the_future = [&](const size_t t_max_iterations, const auto t_power, const bool t_do_abs){
    std::cout<<" New Future: (" << center.x << ", " << center.y << ") iters: " << t_max_iterations << " scale: " << scale << " power: " << t_power << " do_abs: " << t_do_abs << '\n';
    future_pixels<block_size.width, block_size.height>(size, center, scale, pixels, t_max_iterations, t_power, t_do_abs);
  };


  const std::size_t max_max_iterations = 2000;

  const std::size_t max_iteration_increment = 200;
  const std::size_t start_max_iterations = 400;

  std::size_t cur_max_iterations = start_max_iterations;

  see_the_future(cur_max_iterations, power, do_abs);

  while (window.isOpen()) {
    const auto [culled_pixels, none_left] = cull_pixels(pixels, sp);
    if (culled_pixels) {
      texture.loadFromImage(img);
    }

    window.draw(bufferSprite);
    window.display();

    const bool future_changed = 
      [&](){
        bool changed = false;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp)) {
          scale *= 0.9;
          changed = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown)) {
          scale *= 1.1;
          changed = true;
        }
        auto move_offset = scale / 640;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
          move_offset *= 10;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
          center.x -= move_offset;
          changed = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
          center.x += move_offset;
          changed = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
          center.y -= move_offset;
          changed = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
          center.y += move_offset;
          changed = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::P)) {
          if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
            power += 0.1;
          } else {
            power -= 0.1;
          }
          changed = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
          do_abs = !do_abs;
          changed = true;
        }



        return changed;
      }();

    if (future_changed) {
      cur_max_iterations = start_max_iterations;
      see_the_future(cur_max_iterations, power, do_abs);
    } else if (none_left) {
      if (cur_max_iterations < max_max_iterations) {
        cur_max_iterations += max_iteration_increment;
        see_the_future(cur_max_iterations, power, do_abs);
      } else {
        std::this_thread::yield();
      }
    }
  }
}

