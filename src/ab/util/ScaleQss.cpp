#include "ab/util/ScaleQss.hpp"

#include <QDebug>
#include <QRegularExpression>

namespace
{
    void test(QString a, QString b)
    {
        qDebug() << a << (a == b ? "==" : "!=") << b;
    }
}  // namespace

namespace ab
{
    QString scaleQss(const QString& qss, double scale, double nativeScale)
    {
        static auto scaleRegex =
            QRegularExpression("\\b(\\d+(\\.\\d+)?)(dip|px)\\b");

        auto words = QStringList();

        auto it = scaleRegex.globalMatch(qss);
        auto lastEnd = 0;

        while (it.hasNext())
        {
            auto match = it.next();

            // add everything from last match until now
            words.append(qss.mid(lastEnd, match.capturedStart() - lastEnd));
            words.append(QString::number(
                std::round(match.captured(1).toDouble() *
                           (match.captured(3) == "px" ? scale : nativeScale))));
            words.append("px");

            lastEnd = match.capturedEnd();
        }

        words.append(qss.mid(lastEnd));

        return words.join("");
    }

    void testScaleQss()
    {
        test(scaleQss("12dip", 1, 1), "12px");
        test(scaleQss("meme 12dip meme", 1, 1), "meme 12px meme");
        test(scaleQss("12dip", 1, 1.5), "18px");
        test(scaleQss("meme 12dip meme", 1, 2), "meme 24px meme");

        test(scaleQss("12px", 1, 1), "12px");
        test(scaleQss("meme 12px meme", 1, 1), "meme 12px meme");
        test(scaleQss("12px", 1.5, 1), "18px");
        test(scaleQss("meme 12px meme", 2, 1), "meme 24px meme");
    }
}  // namespace ab
