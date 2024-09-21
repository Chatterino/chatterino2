#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <span>

namespace chatterino::testlib {

/// @brief JSON based snapshot/approval tests
///
/// Snapshot tests record the output of some computation based on some @a input.
/// Additionally, users can provide @a params. There isn't any rule on what goes
/// into @a input vs. @a params - a rule of thumb is to put everything that's
/// not directly an input to the target function into @a params (like settings).
///
/// Snapshots are stored in `tests/snapshots/{category}/{name}.json`.
/// `category` can consist of multiple directories (e.g. `foo/bar`).
///
/// @par A minimal example
///
/// ```cpp
/// #include "lib/Snapshot.hpp"
/// #include "Test.hpp"
///
/// #include <array>
/// #include <QStringBuilder>
///
/// namespace testlib = chatterino::testlib;
///
/// constexpr bool UPDATE_SNAPSHOTS = false;
/// constexpr std::array SNAPSHOTS{ "foo", "bar" };
///
/// class ExampleTest : public ::testing::TestWithParam<const char *> {};
///
/// TEST_P(ExampleTest, Run) {
///     auto fixture = testlib::Snapshot::read("category", GetParam());
///     auto output = functionToTest(fixture.input()); // or input{String,Utf8}
///     // if snapshots are supposed to be updated, this will write the output
///     ASSERT_TRUE(fixture.run(output, UPDATE_SNAPSHOTS));
/// }
///
/// INSTANTIATE_TEST_SUITE_P(ExampleInstance, ExampleTest,
///                          testing::ValuesIn(SNAPSHOTS));
///
/// // verify that all snapshots are included
/// TEST(ExampleTest, Integrity) {
///     ASSERT_TRUE(testlib::Snapshot::verifyIntegrity("category", SNAPSHOTS));
///     ASSERT_FALSE(UPDATE_SNAPSHOTS);  // make sure fixtures are actually tested
/// }
/// ```
class Snapshot
{
public:
    Snapshot(const Snapshot &) = delete;
    Snapshot &operator=(const Snapshot &) = delete;

    Snapshot(Snapshot &&) = default;
    Snapshot &operator=(Snapshot &&) = default;
    ~Snapshot() = default;

    /// Read a snapshot
    static Snapshot read(QString category, QString name);

    /// Verifies that all snapshots are included in @a names
    static bool verifyIntegrity(const QString &category,
                                std::span<const char *const> names);

    /// @brief Runs the snapshot test
    ///
    /// If @a updateSnapshots is `false`, this checks that @a got matches the
    /// expected output (#output()).
    /// If @a updateSnapshots is `true`, this sets @a got as the expected
    /// output of this snapshot.
    bool run(const QJsonValue &got, bool updateSnapshots) const;

    QString name() const
    {
        return this->name_;
    }

    QString category() const
    {
        return this->category_;
    }

    QJsonValue input() const
    {
        return this->input_;
    }

    QString inputString() const
    {
        return this->input_.toString();
    }

    QByteArray inputUtf8() const
    {
        return this->input_.toString().toUtf8();
    }

    QJsonValue param(QLatin1String name) const
    {
        return this->params_[name];
    }
    QJsonValue param(const char *name) const
    {
        return this->param(QLatin1String{name});
    }

    QJsonValue output() const
    {
        return this->output_;
    }

private:
    Snapshot(QString category, QString name, const QJsonObject &root);

    void write(const QJsonValue &got) const;

    QString category_;
    QString name_;
    QJsonValue input_;
    QJsonObject params_;
    QJsonValue output_;
};

}  // namespace chatterino::testlib
