#pragma once

namespace chatterino
{
    bool isBigEndian()
    {
        int test = 1;
        char* p = reinterpret_cast<char*>(&test);

        return p[0] == 0;
    }

}  // namespace chatterino
