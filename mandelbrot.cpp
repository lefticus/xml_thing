#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <iostream>
#include <thread>
#include <future>

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
    if ((x*x) + (y*y) > (2.0*2.0) && stop_iteration == max_iteration) {
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
    img.setPixel(t_point.x, t_point.y, to_sf_color(t_color));
  }
};

template<std::size_t BlockWidth, std::size_t BlockHeight>
struct ImageBlock
{
  using Row = std::array<Color<double>, BlockWidth>;
  Point<std::size_t> upper_left;
  std::array<Row, BlockHeight> image;

  static constexpr const auto width = BlockWidth;
  static constexpr const auto height = BlockHeight;
};

template<size_t BlockWidth, size_t BlockHeight, typename SizeType, typename CenterType, typename Container>
void future_pixels(const Size<SizeType> &t_size, const Point<CenterType> &t_center,
                   const double t_scale, Container &t_container)
{
  for (std::size_t y = 0; y < t_size.height; y += BlockHeight)
  {
    for (std::size_t x = 0; x < t_size.width; x += BlockWidth)
    {
      t_container.push_back(
          std::async(
            [p = Point{x, y}, t_size, t_center, t_scale](){
              using Row = std::array<Color<double>, BlockWidth>;
              ImageBlock<BlockWidth, BlockHeight> image{p};

              for (std::size_t x = 0; x < image.width; ++x) {
                for (std::size_t y = 0; y < image.height; ++y) {
                  image.image[y][x] = get_color(Point{p.x + x, p.y + y}, t_center, t_size, t_scale);
                }
              }
              return image;
            }
          )
        );
    }
  }
}

template<typename Container>
bool cull_pixels(Container &t_container, SetPixel &t_set_pixel)
{
  const auto did_something = !t_container.empty();

  std::for_each(t_container.begin(), t_container.end(), 
      [&](auto &f){
        const auto result = f.get();

        for (std::size_t x = 0; x < result.width; ++x) {
          for (std::size_t y = 0; y < result.height; ++y) {
            t_set_pixel.set_pixel(
                Point{result.upper_left.x + x, result.upper_left.y + y},
                result.image[y][x]
              );
          }
        }
      }
    );

  t_container.clear();
  return did_something;
}

int main() {
  const Size size{640u, 640u};
  constexpr const Size block_size{160u, 160u};

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
  auto scale = 0.01;

  std::vector<std::future<ImageBlock<block_size.width, block_size.height>>> pixels;

  auto see_the_future = [&](){
    future_pixels<block_size.width, block_size.height>(size, center, scale, pixels);
  };

  see_the_future();

  while (window.isOpen()) {
    if (cull_pixels(pixels, sp)) {
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

        return changed;
      }();

    if (future_changed) {
      see_the_future();
    }
  }

}

