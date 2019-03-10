#pragma once

#include <QTimer>
#include <chrono>
#include <optional>
#include <unordered_map>

namespace chatterino::ui
{
    enum class PauseReason {
        Mouse,
        Selection,
        DoubleClick,
    };

    class Pauser : public QObject
    {
        Q_OBJECT

        using Clock = std::chrono::steady_clock;

    public:
        explicit Pauser(QObject* parent = nullptr);

        [[nodiscard]] bool pausable() const;
        void setPausable(bool);
        [[nodiscard]] bool paused() const;
        void pause(PauseReason, std::optional<unsigned> msecs = {});
        void unpause(PauseReason);

    signals:
        void unpaused();

    private slots:
        void timeout();

    private:
        void updatePauseTimer();

        bool pausable_{};
        QTimer pauseTimer_;
        std::unordered_map<PauseReason, std::optional<Clock::time_point>>
            pauses_;
        std::optional<Clock::time_point> pauseEnd_;
    };
}  // namespace chatterino::ui
