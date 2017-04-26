#include <regex>
#include <variant>
#include <map>
#include <vector>
#include <iostream>
#include <stdexcept>

struct DOMObject
{
  DOMObject() = default;
  DOMObject(std::string t_name, std::map<std::string, std::string> t_attrs)
    : name(std::move(t_name)), attributes(std::move(t_attrs))
  { }
  std::string name;
  std::map<std::string, std::string> attributes;
  using DOMElement = std::variant<DOMObject, std::string>;
  std::vector<DOMElement> children;
};

template<typename Char>
std::map<std::string, std::string> parse_attributes(const std::sub_match<Char> &chars)
{
  static const std::regex attribute(R"(\s+(\S+)\s*=\s*('|")(.*?)\2)");
  std::map<std::string, std::string> retval;

  std::transform(
      std::regex_iterator(chars.first, chars.second, attribute),
      std::regex_iterator<Char>{},
      std::inserter(retval, retval.end()),
      [](const auto &match){ return std::pair{match.str(1), match.str(3)}; }
    );

  return retval;
}

template<typename Callable, typename Tuple>
bool any_of_elem(Tuple &&tuple, Callable &&func)
{
  return std::apply([&func](auto&&... xs) {
     return (func(std::forward<decltype(xs)>(xs)) || ...);
  }, std::forward<Tuple>(tuple));
}

void parse(std::string_view chars, DOMObject &parent)
{
  constexpr auto node_match = [](const auto &match, auto &parent){
    parse( std::string_view(match[3].first, match[3].length()),
        std::get<DOMObject>(parent.children.emplace_back(DOMObject(match.str(1), parse_attributes(match[2]))))
        );
  };
  constexpr auto empty_node_match = [](const auto &match, auto &parent){
    parent.children.emplace_back(DOMObject(match.str(1), parse_attributes(match[2])));
  };
  constexpr auto cdata_match = [](const auto &match, auto &parent){ parent.children.emplace_back(match.str(0)); };
  constexpr auto whitespace_match = [](const auto &, auto &){ };

  static const std::tuple events = { 
    std::pair{ std::regex{R"(^<(\S+)(\s.*?)?>((.|\s)*?)<\/\1>)"}, node_match       },
    std::pair{ std::regex{R"(^<(\S+)(\s.*?)?\/>)"}              , empty_node_match },
    std::pair{ std::regex{R"(^[^<]+)"}                          , cdata_match      },
    std::pair{ std::regex{R"(^\s+)"}                            , whitespace_match }
  };

  auto find_match = [](const auto &parser, auto &chars, auto &parent){
    if (std::cmatch results;
        std::regex_search(chars.begin(), chars.end(), results, parser.first))
    {
      chars.remove_prefix(results.length(0));
      parser.second(results, parent);
      return true;
    } else {
      return false;
    }
  };

  while (!chars.empty()) {
    const auto matched = any_of_elem(events,
          [&](const auto &event) { return find_match(event, chars, parent); } );
    if (!matched) {
      throw std::runtime_error("Mismatched Parse");
    }
  }
}

DOMObject parse(std::string_view chars)
{
  DOMObject topLevel;
  parse(chars, topLevel);
  return topLevel;
}

void print(const DOMObject &obj, int indent = 0)
{
  std::string indent_str = std::string(indent * 2, ' ');
  std::cout << indent_str << "Object: " << obj.name;

  for (const auto &[key, value] : obj.attributes) {
    std::cout << " (" << key << ',' << value << ')';
  }
  std::cout << '\n';
  std::cout << indent_str; 
  
  for(const auto &child : obj.children) {
    if (const auto *val = std::get_if<std::string>(&child); val != nullptr) {
      std::cout << indent_str << "CData: '" << *val << "'\n";
    } else if (const auto *val = std::get_if<DOMObject>(&child); val != nullptr) {
      print(*val, indent + 1);
    }
  }
}

int main()
{
  const auto thing1 = R"(<doc param='value' param2="value2"><other_thing></other_thing></doc> some s
  trings
  <tag value="something" value2=' "another" '/>
  stuff<stuff></stuff>)";

  DOMObject doc = parse(thing1);
  print(doc);
}


