#ifndef UTILS_H
#define UTILS_H

#include <QDate>
#include <QString>
#include <QVBoxLayout>

#include <optional>

class QSettings;
class QScrollArea;
class QWidget;

namespace Paths {
bool portable();
QString configDir();
QString configPath();
QString dataDir();
QString todoFile();
QString doneFile();
QString iconsDir();
QString translationsDir();
QSettings *settings();
}

namespace FileUtils {
std::optional<QString> readFile(const QString &path);
bool writeFile(const QString &path, const QString &content);
bool appendFile(const QString &path, const QString &content);
}

namespace LayoutUtils {
void clearRows(QVBoxLayout *layout);
void adjustRowHeights(QVBoxLayout *layout);

template <typename RowType, typename Func>
void forEachRow(QVBoxLayout *layout, Func &&func) {
  for (int i = 0; i < layout->count(); ++i) {
    QLayoutItem *item = layout->itemAt(i);
    if (!item || !item->widget()) continue;
    auto *row = qobject_cast<RowType *>(item->widget());
    if (row) func(row);
  }
}
}

namespace UiUtils {
QWidget *setupListScrollArea(QScrollArea *area);
}

namespace DateUtils {
QString currentDateStr();
QString stripDate(const QString &line);
QDate dateFromLine(const QString &line);
}

#endif  // UTILS_H
