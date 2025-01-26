#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include "http_client/Utils.h"

namespace nvd {
namespace utils {

// todo - check this base64_encode based openssl API VS base64Encode
// Function to Base64 encode credentials
// std::string base64_encode(const std::string& input) 
// {
//     BIO* b64 = BIO_new(BIO_f_base64());
//     BIO* bio = BIO_new(BIO_s_mem());
//     b64 = BIO_push(b64, bio);
//     BIO_write(b64, input.c_str(), static_cast<int>(input.size()));
//     BIO_flush(b64);

//     BUF_MEM* buffer_ptr;
//     BIO_get_mem_ptr(b64, &buffer_ptr);

//     std::string encoded(buffer_ptr->data, buffer_ptr->length - 1); // Exclude the trailing '\n'
//     BIO_free_all(b64);
//     return encoded;
// }

std::string base64Encode(const std::string &in)
{
    std::string out;
    std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    int i = 0;
    int j = 0;

    size_t in_len = in.size();
    unsigned char *bytes_to_encode = (unsigned char *)in.c_str();

    while (in_len--)
    {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) | ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) | ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                out += base64_chars[char_array_4[i]];

            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) | ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) | ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            out += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            out += "=";
    }

    return out;
}

}} // namespace