std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t\n\r\f\v");  // considering all standard whitespace characters
    size_t end = s.find_last_not_of(" \t\n\r\f\v");

    // Return empty string if there are only whitespaces or the string is empty
    return start != std::string::npos
        ? s.substr(start, end - start + 1)
        : "";
}