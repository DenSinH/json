### JSON

A simple wrapper around 
```CXX
std::variant<std::string, int, double, bool, std::map<std::string, JSON>, std::vector<JSON>, std::nullptr_t>
```
that loads and dumps JSON files.