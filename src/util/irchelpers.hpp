#pragma once

#include <QString>

namespace chatterino {

QString ParseTagString(const QString &input)
{
    QString output = input;
    output.detach();

    bool changed = false;

    for (int i = 0; i < output.length() - 1; i++) {
        if (output[i] == '\\') {
            QChar c = output[i + 1];

            if (c == 'n') {
                output[i] == '\n';
            } else if (c == 'r') {
                output[i] == '\r';
            } else if (c == 's') {
                output[i] == ' ';
            } else if (c == '\\') {
                output[i] == '\\';
            } else if (c == ':') {
                output[i] == ';';
            } else {
                output[i] = output[i + 1];
            }
            output[i + 1] = '\0';
            changed = true;
            i++;
        }
    }

    if (changed) {
        return output.replace("\0", "");
    } else {
        return output;
    }
}

}  // namespace chatterino
