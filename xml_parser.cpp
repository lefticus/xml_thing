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
  {
  }
  std::string name;
  std::map<std::string, std::string> attributes;
  using DOMElement = std::variant<DOMObject, std::string>;
  std::vector<DOMElement> children;
};

template<typename T>
void dump_match(const T &matches) {
  std::cout << "Match: " << matches.size() << '\n';

  for (size_t i = 0; i < matches.size(); ++i) {
    auto sub_match = matches[i];
    std::cout << "  " << i << ':' << sub_match.str() << '\n';
  }
}


template<typename Char>
std::map<std::string, std::string> parse_attributes(const std::sub_match<Char> &chars)
{
  static const std::regex attribute(R"(\s+(\S+)\s*=\s*('|")(.*?)\2)");
  std::map<std::string, std::string> retval;
  using regex_iterator = std::regex_iterator<Char>;


  for (auto match = regex_iterator(chars.first, chars.second, attribute);
       match != decltype(match)();
       ++match)
  {
    retval.emplace(match->str(1), match->str(3));
  }
 
  return retval;
}

void parse(std::string_view chars, DOMObject &parent)
{
  enum class match_type { tag, empty_tag, cdata, whitespace };

  static const std::pair<match_type, std::regex> regexes[] = {
    std::pair{match_type::tag,        std::regex{R"(^<(\S+)(\s.*?)?>((.|\s)*?)<\/\1>)"}},
    std::pair{match_type::empty_tag,  std::regex{R"(^<(\S+)(\s.*?)?\/>)"}},
    std::pair{match_type::cdata,      std::regex{R"(^[^<]+)"}},
    std::pair{match_type::whitespace, std::regex{R"(^\s+)"}}
  };

  const auto find_match = [](const auto &regex, auto &chars){
    std::pair<bool, std::cmatch> results;
    results.first = std::regex_search(chars.begin(), chars.end(), results.second, regex);
    chars.remove_prefix(results.second.length(0));
    return results;
  };

  while (!chars.empty()) {
    for(const auto regex : regexes) {
      if (const auto [found, match] = find_match(regex.second, chars); found)
      {
        dump_match(match);
        switch (regex.first) {
          case match_type::tag:
            parse(
                std::string_view(match[3].first, match[3].length()),
                std::get<DOMObject>(parent.children.emplace_back(DOMObject(match.str(1), parse_attributes(match[2]))))
                );
            break;
          case match_type::empty_tag:
            parent.children.emplace_back(DOMObject(match.str(1), parse_attributes(match[2])));
            break;
          case match_type::cdata:
            parent.children.emplace_back(match.str(0));
          case match_type::whitespace:
            break;
        }
      }
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
  stuff <stuff></stuff>)";

  DOMObject doc = parse(thing1);
  print(doc);
}
