#pragma once

#include <IrcMessage>
#include <QString>
#include <QTimeZone>

namespace chatterino {

inline QString parseTagString(const QString &input)
{
    QString output = input;
    output.detach();

    auto length = output.length();

    for (int i = 0; i < length - 1; i++)
    {
        if (output[i] == '\\')
        {
            QChar c = output[i + 1];

            switch (c.cell())
            {
                case 'n': {
                    output.replace(i, 2, '\n');
                }
                break;

                case 'r': {
                    output.replace(i, 2, '\r');
                }
                break;

                case 's': {
                    output.replace(i, 2, ' ');
                }
                break;

                case '\\': {
                    output.replace(i, 2, '\\');
                }
                break;

                case ':': {
                    output.replace(i, 2, ';');
                }
                break;

                default: {
                    output.remove(i, 1);
                }
                break;
            }

            length--;
        }
    }

    return output;
}

QDateTime calculateMessageTime(const Communi::IrcMessage *message);

// "foo/bar/baz,tri/hard" can be a valid badge-info tag
// In that case, valid map content should be 'split by slash' only once:
// {"foo": "bar/baz", "tri": "hard"}
inline std::pair<QString, QString> slashKeyValue(const QString &kvStr)
{
    return {
        // part before first slash (index 0 of section)
        kvStr.section('/', 0, 0),
        // part after first slash (index 1 of section)
        kvStr.section('/', 1, -1),
    };
}

}  // namespace chatterino
