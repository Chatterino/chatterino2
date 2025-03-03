#include "lib/Snapshot.hpp"

#include "common/Literals.hpp"

#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStringBuilder>

namespace {

using namespace chatterino::literals;

bool compareJson(const QJsonValue &expected, const QJsonValue &got,
                 const QString &context)
{
    if (expected == got)
    {
        return true;
    }
    if (expected.type() != got.type())
    {
        qWarning() << context
                   << "- mismatching type - expected:" << expected.type()
                   << "got:" << got.type();
        return false;
    }
    switch (expected.type())
    {
        case QJsonValue::Array: {
            auto expArr = expected.toArray();
            auto gotArr = got.toArray();
            if (expArr.size() != gotArr.size())
            {
                qWarning() << context << "- Mismatching array size - expected:"
                           << expArr.size() << "got:" << gotArr.size();
                return false;
            }
            for (QJsonArray::size_type i = 0; i < expArr.size(); i++)
            {
                if (!compareJson(expArr[i], gotArr[i],
                                 context % '[' % QString::number(i) % ']'))
                {
                    return false;
                }
            }
        }
        break;  // unreachable
        case QJsonValue::Object: {
            auto expObj = expected.toObject();
            auto gotObj = got.toObject();
            if (expObj.size() != gotObj.size())
            {
                qWarning() << context << "- Mismatching object size - expected:"
                           << expObj.size() << "got:" << gotObj.size();
                return false;
            }

            for (auto it = expObj.constBegin(); it != expObj.constEnd(); it++)
            {
                if (!gotObj.contains(it.key()))
                {
                    qWarning() << context << "- Object doesn't contain key"
                               << it.key();
                    return false;
                }
                if (!compareJson(it.value(), gotObj[it.key()],
                                 context % '.' % it.key()))
                {
                    return false;
                }
            }
        }
        break;
        case QJsonValue::Null:
        case QJsonValue::Bool:
        case QJsonValue::Double:
        case QJsonValue::String:
        case QJsonValue::Undefined:
            break;
    }

    qWarning() << context << "- expected:" << expected << "got:" << got;
    return false;
}

void mergeJson(QJsonObject &base, const QJsonObject &additional)
{
    for (auto it = additional.begin(); it != additional.end(); it++)
    {
        auto ref = base[it.key()];

        if (ref.isArray())
        {
            // there's no way of pushing to the array without detaching first
            auto arr = ref.toArray();
            if (!it->isArray())
            {
                throw std::runtime_error("Mismatched types");
            }

            // append all additional values
            auto addArr = it->toArray();
            for (auto v : addArr)
            {
                arr.append(v);
            }
            ref = arr;
            continue;
        }

        if (ref.isObject())
        {
            // same here, detach first and overwrite
            auto obj = ref.toObject();
            if (!it->isObject())
            {
                throw std::runtime_error("Mismatched types");
            }
            mergeJson(obj, it->toObject());
            ref = obj;
            continue;
        }

        ref = it.value();  // overwrite for simple types/non-existent keys
    }
}

QDir baseDir(const QString &category)
{
    QDir snapshotDir(QStringLiteral(__FILE__));
    snapshotDir.cd("../../../snapshots/");
    snapshotDir.cd(category);
    return snapshotDir;
}

QString filePath(const QString &category, const QString &name)
{
    return baseDir(category).filePath(name);
}

}  // namespace

namespace chatterino::testlib {

std::unique_ptr<Snapshot> Snapshot::read(QString category, QString name)
{
    if (!name.endsWith(u".json"))
    {
        name.append(u".json");
    }

    QFile file(filePath(category, name));
    if (!file.open(QFile::ReadOnly))
    {
        throw std::runtime_error("Failed to open file");
    }
    auto content = file.readAll();
    file.close();
    const auto doc = QJsonDocument::fromJson(content).object();

    return std::unique_ptr<Snapshot>(
        new Snapshot(std::move(category), std::move(name), doc));
}

QStringList Snapshot::discover(const QString &category)
{
    auto files =
        baseDir(category).entryList(QDir::NoDotAndDotDot | QDir::Files);
    for (auto &file : files)
    {
        file.remove(".json");
    }
    return files;
}

QStringList Snapshot::discoverNested(const QString &category)
{
    auto directories =
        baseDir(category).entryList(QDir::NoDotAndDotDot | QDir::Dirs);
    QStringList all;
    for (const auto &dir : directories)
    {
        auto d = baseDir(category);
        d.cd(dir);
        auto files = d.entryList(QDir::NoDotAndDotDot | QDir::Files);
        for (const auto &file : files)
        {
            QStringView view(file);
            if (view.endsWith(u".json"))
            {
                view = view.sliced(0, view.size() - 5);
            }
            all.append(dir % u'/' % view);
        }
    }
    return all;
}

bool Snapshot::run(const QJsonValue &got, bool updateSnapshots) const
{
    if (updateSnapshots)
    {
        this->write(got);
        return true;
    }

    return compareJson(this->output_, got, QStringLiteral("output"));
}

Snapshot::Snapshot(QString category, QString name, const QJsonObject &root)
    : category_(std::move(category))
    , name_(std::move(name))
    , input_(root["input"_L1])
    , params_(root["params"_L1].toObject())
    , settings_(root["settings"_L1].toObject())
    , output_(root["output"_L1])
{
}

void Snapshot::write(const QJsonValue &got) const
{
    QFile file(filePath(this->category_, this->name_));
    if (!file.open(QFile::WriteOnly))
    {
        throw std::runtime_error("Failed to open file");
    }

    QJsonObject obj{
        {"input"_L1, this->input_},
        {"output"_L1, got},
    };
    if (!this->params_.isEmpty())
    {
        obj.insert("params"_L1, this->params_);
    }
    if (!this->settings_.isEmpty())
    {
        obj.insert("settings"_L1, this->settings_);
    }

    file.write(QJsonDocument{obj}.toJson());
    file.close();
}

QByteArray Snapshot::mergedSettings(const QByteArray &base) const
{
    auto baseDoc = QJsonDocument::fromJson(base);
    if (!baseDoc.isObject())
    {
        throw std::runtime_error("Invalid base settings");
    }
    auto baseObj = baseDoc.object();
    mergeJson(baseObj, this->settings_);

    baseDoc.setObject(baseObj);
    return baseDoc.toJson(QJsonDocument::Compact);
}

}  // namespace chatterino::testlib
