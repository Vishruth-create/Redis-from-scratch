#include <string>
std::string to_upper(const std::string &str)
{
    std::string upper{};
    for(int c=0 ; c < str.length(); c++)
    {
        if(str[c]>=97 && str[c] <=122)
        {
            upper+=(str[c]-32);
        }
        else upper+=str[c];
    }
    return upper;
}