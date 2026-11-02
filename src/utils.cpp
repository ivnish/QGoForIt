#include "utils.h"
#include "baserowwidget.h"
#include "config.h"
#include "constants.h"

#include <QCoreApplication>
#include <QDate>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSettings>
#include <QStandardPaths>
#include <QWidget>

bool Paths::portable() {
  static const bool isPortable = QFileInfo::exists(
      QCoreApplication::applicationDirPath() + "/portable");
  return isPortable;
}

QSettings *Paths::settings() {
  static QSettings instance(configPath(), QSettings::IniFormat);
  return &instance;
}

QString Paths::configDir() {
  if (portable()) {
    return QCoreApplication::applicationDirPath();
  }
  return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
         "/QGoForIt";
}

QString Paths::configPath() {
  return configDir() + "/config.ini";
}

QString Paths::dataDir() {
  if (portable()) {
    return QCoreApplication::applicationDirPath();
  }
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString Paths::todoFile() {
  return dataDir() + "/todo.txt";
}

QString Paths::doneFile() {
  return dataDir() + "/done.txt";
}

QString Paths::iconsDir() {
  if (portable()) {
    return QCoreApplication::applicationDirPath() + "/icons";
  }
  if (qEnvironmentVariableIsSet("APPIMAGE")) {
    return QCoreApplication::applicationDirPath() +
           "/../share/QGoForIt/icons";
  }
  return QString(APP_INSTALL_PREFIX) + "/share/QGoForIt/icons";
}

QString Paths::translationsDir() {
  if (portable()) {
    return QCoreApplication::applicationDirPath() + "/translations";
  }
  if (qEnvironmentVariableIsSet("APPIMAGE")) {
    return QCoreApplication::applicationDirPath() +
           "/../share/QGoForIt/translations";
  }
  return QString(APP_INSTALL_PREFIX) + "/share/QGoForIt/translations";
}

QWidget *UiUtils::setupListScrollArea(QScrollArea *area) {
  area->setWidgetResizable(true);
  area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  area->setFrameShape(QFrame::NoFrame);

  auto *container = new QWidget;
  container->setAutoFillBackground(true);
  QPalette containerPalette = container->palette();
  containerPalette.setColor(QPalette::Window, Qt::white);
  container->setPalette(containerPalette);

  QPalette vpPalette = area->viewport()->palette();
  vpPalette.setColor(QPalette::Window, Qt::white);
  area->viewport()->setPalette(vpPalette);
  area->viewport()->setAutoFillBackground(true);

  area->setWidget(container);
  return container;
}

std::optional<QString> FileUtils::readFile(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "Failed to open" << path << ":" << file.errorString();
    return std::nullopt;
  }
  return file.readAll();
}

bool FileUtils::writeFile(const QString &path, const QString &content) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qWarning() << "Failed to save" << path << ":" << file.errorString();
    return false;
  }
  file.write(content.toUtf8());
  return true;
}

bool FileUtils::appendFile(const QString &path, const QString &content) {
  QFile file(path);
  if (!file.open(QIODevice::Append | QIODevice::Text)) {
    qWarning() << "Failed to append to" << path << ":" << file.errorString();
    return false;
  }
  file.write(content.toUtf8());
  return true;
}

void LayoutUtils::clearRows(QVBoxLayout *layout) {
  for (int i = layout->count() - 1; i >= 0; --i) {
    QLayoutItem *item = layout->itemAt(i);
    if (!item || !item->widget()) continue;
    auto *row = qobject_cast<BaseRowWidget *>(item->widget());
    if (!row) continue;
    layout->removeWidget(row);
    row->deleteLater();
  }
}

void LayoutUtils::adjustRowHeights(QVBoxLayout *layout) {
  forEachRow<BaseRowWidget>(
      layout, [](BaseRowWidget *row) { row->adjustHeight(); });
}

namespace {
struct LinePrefix {
  QDate date;
  int textPos = 0;
};

LinePrefix parseLinePrefix(const QString &line) {
  static const QRegularExpression re(
      QStringLiteral("^(?:%1)?(?:(\\d{2}\\.\\d{2}\\.\\d{4}) )?")
          .arg(Formats::DONE_PREFIX));
  const QRegularExpressionMatch match = re.match(line);
  LinePrefix prefix;
  prefix.date = QDate::fromString(match.captured(1), Formats::DATE_FORMAT);
  prefix.textPos = static_cast<int>(match.capturedLength());
  return prefix;
}
}  // namespace

QString DateUtils::currentDateStr() {
  return QDate::currentDate().toString(Formats::DATE_FORMAT);
}

QString DateUtils::stripDate(const QString &line) {
  return line.mid(parseLinePrefix(line).textPos);
}

QDate DateUtils::dateFromLine(const QString &line) {
  return parseLinePrefix(line).date;
}
