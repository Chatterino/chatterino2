#pragma once

#include <QString>
#include <functional>
#include <set>
#include <unordered_map>

namespace chatterino
{
    class Prefix
    {
    public:
        Prefix(const QString& string);
        bool operator==(const Prefix& other) const;
        bool isStartOf(const QString& string) const;

    private:
        QChar first;
        QChar second;

        friend struct std::hash<Prefix>;
    };
}  // namespace chatterino

namespace std
{
    template <>
    struct hash<chatterino::Prefix>
    {
        size_t operator()(const chatterino::Prefix& prefix) const
        {
            return (size_t(prefix.first.unicode()) << 16) |
                   size_t(prefix.second.unicode());
        }
    };
}  // namespace std

namespace chatterino
{
    class UsernameSet
    {
    public:
        static constexpr int PrefixLength = 2;

        using Iterator = std::set<QString>::iterator;
        using ConstIterator = std::set<QString>::const_iterator;

        class Range
        {
        public:
            Range(ConstIterator start, ConstIterator end);

            ConstIterator begin();
            ConstIterator end();

        private:
            ConstIterator start_;
            ConstIterator end_;
        };

        ConstIterator begin() const;
        ConstIterator end() const;
        Range subrange(const Prefix& prefix) const;

        std::set<QString>::size_type size() const;

        std::pair<Iterator, bool> insert(const QString& value);
        std::pair<Iterator, bool> insert(QString&& value);

    private:
        void insertPrefix(const QString& string);

        std::set<QString> items;
        std::unordered_map<Prefix, QString> firstKeyForPrefix;
    };
}  // namespace chatterino
