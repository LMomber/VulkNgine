#pragma once

#include "pch.h"

class ImGuiConsoleBuf : public std::streambuf
{
public:
    std::string buffer;
    std::streambuf* originalBuf = nullptr;

    ~ImGuiConsoleBuf()
    {
        if (std::cout.rdbuf() == this)
            std::cout.rdbuf(originalBuf);
    }

protected:
    int_type overflow(int_type ch) override
    {
        if (ch != traits_type::eof())
        {
            buffer += static_cast<char>(ch);

            if (originalBuf)
                originalBuf->sputc(static_cast<char>(ch));
        }

        return ch;
    }

    std::streamsize xsputn(const char* s, std::streamsize count) override
    {
        buffer.append(s, count);

        if (originalBuf)
            originalBuf->sputn(s, count);

        return count;
    }
};