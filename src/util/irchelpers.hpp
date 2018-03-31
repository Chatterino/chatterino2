#pragma once

#include <QString>

namespace chatterino {
namespace util {

inline QString ParseTagString(const QString &input)
{
    QString output = input;
    output.detach();

    bool changed = false;

    for (int i = 0; i < output.length() - 1; i++) {
        if (output[i] == '\\') {
            QChar c = output[i + 1];

            switch (c.cell()) {
                case 'n': {
                    output[i] = '\n';
                } break;

                case 'r': {
                    output[i] = '\r';
                } break;

                case 's': {
                    output[i] = ' ';
                } break;

                case '\\': {
                    output[i] = '\\';
                } break;

                case ':': {
                    output[i] = ';';
                } break;

                default: {
                    output[i] = output[i + 1];
                } break;
            }

            output[i + 1] = '\0';
            changed = true;
            i++;
        }
    }

    if (changed) {
        return output.replace("\0", "");
    }

    return output;
}

}  // namespace util
}  // namespace chatterino
