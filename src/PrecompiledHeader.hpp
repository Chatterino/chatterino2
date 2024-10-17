#ifdef __cplusplus
#    include <boost/circular_buffer.hpp>
#    include <boost/current_function.hpp>
#    include <boost/foreach.hpp>
#    include <boost/signals2.hpp>
#    include <IrcCommand>
#    include <IrcConnection>
#    include <IrcMessage>
#    include <nonstd/expected.hpp>
#    include <pajlada/serialize.hpp>
#    include <pajlada/settings/setting.hpp>
#    include <pajlada/settings/settinglistener.hpp>
#    include <pajlada/signals/connection.hpp>
#    include <pajlada/signals/signal.hpp>
#    include <QAbstractListModel>
#    include <QAction>
#    include <QApplication>
#    include <QBrush>
#    include <QBuffer>
#    include <QByteArray>
#    include <QCheckBox>
#    include <QClipboard>
#    include <QColor>
#    include <QComboBox>
#    include <QDateTime>
#    include <QDebug>
#    include <QDesktopServices>
#    include <QDialog>
#    include <QDialogButtonBox>
#    include <QDir>
#    include <QElapsedTimer>
#    include <QFile>
#    include <QFileDialog>
#    include <QFileInfo>
#    include <QFlags>
#    include <QFont>
#    include <QFontMetrics>
#    include <QFormLayout>
#    include <QGroupBox>
#    include <QHBoxLayout>
#    include <QHeaderView>
#    include <QIcon>
#    include <QImageReader>
#    include <QJsonArray>
#    include <QJsonDocument>
#    include <QJsonObject>
#    include <QJsonValue>
#    include <QKeyEvent>
#    include <QLabel>
#    include <QLayout>
#    include <QLineEdit>
#    include <QList>
#    include <QListView>
#    include <QListWidget>
#    include <QMap>
#    include <QMenu>
#    include <QMessageBox>
#    include <QMimeData>
#    include <QMouseEvent>
#    include <QMutex>
#    include <QMutexLocker>
#    include <QNetworkAccessManager>
#    include <QNetworkReply>
#    include <QNetworkRequest>
#    include <QObject>
#    include <QPainter>
#    include <QPainterPath>
#    include <QPaintEvent>
#    include <QPalette>
#    include <QPixmap>
#    include <QPoint>
#    include <QProcess>
#    include <QPropertyAnimation>
#    include <QPushButton>
#    include <QRadialGradient>
#    include <QRect>
#    include <QRegularExpression>
#    include <QRunnable>
#    include <QScroller>
#    include <QShortcut>
#    include <QSizePolicy>
#    include <QSlider>
#    include <QSpinBox>
#    include <QStandardPaths>
#    include <QString>
#    include <QStyle>
#    include <QStyleOption>
#    include <QTabWidget>
#    include <QTextEdit>
#    include <QtGlobal>
#    include <QThread>
#    include <QThreadPool>
#    include <QTime>
#    include <QTimer>
#    include <QUrl>
#    include <QUuid>
#    include <QVariant>
#    include <QVBoxLayout>
#    include <QVector>
#    include <QWheelEvent>
#    include <QWidget>
#    include <rapidjson/document.h>
#    include <rapidjson/error/en.h>
#    include <rapidjson/error/error.h>

#    include <algorithm>
#    include <cassert>
#    include <chrono>
#    include <cinttypes>
#    include <climits>
#    include <cmath>
#    include <concepts>
#    include <cstdint>
#    include <ctime>
#    include <functional>
#    include <future>
#    include <list>
#    include <map>
#    include <memory>
#    include <mutex>
#    include <optional>
#    include <random>
#    include <set>
#    include <string>
#    include <thread>
#    include <tuple>
#    include <type_traits>
#    include <unordered_map>
#    include <unordered_set>
#    include <vector>

#    ifdef CHATTERINO_HAVE_PLUGINS
#        include <sol/sol.hpp>
#    endif

#    ifndef UNUSED
#        define UNUSED(x) (void)(x)
#    endif

#    ifndef ATTR_UNUSED
#        ifdef Q_OS_WIN
#            define ATTR_UNUSED
#        else
#            define ATTR_UNUSED __attribute__((unused))
#        endif
#    endif

#endif
