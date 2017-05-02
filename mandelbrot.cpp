#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <iostream>

template<typename T>
struct Point {
  T x{};
  T y{};
};

template<typename T>
Point(T x, T y) -> Point<T>;


template<typename T>
struct Size {
  T width{};
  T height{};
};

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

template<typename PointType, typename CenterType, typename SizeType>
auto get_color(const Point<PointType> &t_point, const Point<CenterType> &t_center, 
               const Size<SizeType> &t_size, const double t_scale) noexcept
{
  auto xscaled = t_point.x/(t_size.width/t_scale) + (t_center.x - (t_scale/2.0 ));
  auto yscaled = t_point.y/(t_size.height/t_scale) + (t_center.y - (t_scale/2.0 ));

  auto x = xscaled;
  auto y = yscaled;

  auto iteration = 0;
  auto max_iteration = 2000;

  auto stop_iteration = max_iteration;

  while ( iteration < stop_iteration )
  {
    if ((x*x) + (y*y) > (2.0*2.0) && stop_iteration == max_iteration)
    {
      stop_iteration = iteration + 5;
    }

    auto xtemp = (x*x) - (y*y) + xscaled;
    y = (2.0*x*y) + yscaled;
    x = xtemp;
    ++iteration;
  }

  if (iteration == max_iteration)
  {
    return Color{0.0,0.0,0.0};
  } else {
    const auto value = ((iteration + 1) - (std::log(std::log(std::abs(x * y))))/std::log(2.0)); 
    const auto colorval = static_cast<int>(std::floor(value * 10.0));

    if (colorval < 256)
    {
      return Color{colorval/256.0, 0.0, 0.0};
    } else {
      const auto colorband = ((colorval - 256) % 1024) / 256;
      const auto mod256 = colorval % 256;
      if (colorband == 0) {
        return Color{1.0, mod256 / 255.0, 0.0};
      } else if (colorband == 1) {
        return Color{1.0, 1.0, mod256 / 255.0};
      } else if (colorband == 2) {
        return Color{1.0, 1.0, 1.0 - (mod256 / 255.0)};
      } else {
        return Color{1.0, 1.0 - (mod256 / 255.0), 0.0};
      }
    }
  }
}


struct SetPixel
{
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
    const auto color = to_sf_color(t_color);
//    std::cout << "new r: " << static_cast<int>(color.r) << " g: " << static_cast<int>(color.g) << " b: " << static_cast<int>(color.b) << '\n';
    img.setPixel(t_point.x, t_point.y, to_sf_color(t_color));
  }
};

int main()
{
  const Size size{600, 600};
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
  // run the main loop
  for (auto y = 0; y < size.height; ++y)
  {
    for (auto x = 0; x < size.width; ++x)
    {
      const auto p = Point{x, y};
      const auto color = get_color(p, center, size, .250);
  //    std::cout << "r: " << color.r << " g: " << color.g << " b: " << color.b << '\n';
      sp.set_pixel(p, color);
  //    std::cout << "x: " << x << " y: " << y << '\n';
    }
    texture.loadFromImage(img);
    window.draw(bufferSprite);
    window.display();
  }

  while (window.isOpen())
  {
    window.draw(bufferSprite);
    window.display();
  }
}

