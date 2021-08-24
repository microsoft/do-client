#pragma once

#include <cctype>
#include <string>

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

class Url
{
public:
    // Unreserved characters are those that are allowed in a URI but do not have a reserved purpose
    static bool IsUnreserved(int c)
    {
        return isalnum(static_cast<char>(c)) || c == '-' || c == '.' || c == '_' || c == '~';
    }

    static std::string EncodeDataString(const std::string& input)
    {
        // Credits: cpprestsdk
        // https://github.com/microsoft/cpprestsdk/blob/master/Release/src/uri/uri.cpp

        const char* const hex = "0123456789ABCDEF";
        std::string encoded;
        for (auto c : input)
        {
            // for utf8 encoded string, char ASCII can be greater than 127.
            const int ch = static_cast<unsigned char>(c);
            if (!IsUnreserved(ch))
            {
                encoded.push_back('%');
                encoded.push_back(hex[(ch >> 4) & 0xF]);
                encoded.push_back(hex[ch & 0xF]);
            }
            else
            {
                encoded.push_back(static_cast<char>(ch));
            }
        }
        return encoded;
    }

};

}
}
} // namespace microsoft::deliveryoptimization::details
