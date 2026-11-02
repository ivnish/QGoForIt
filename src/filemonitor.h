#ifndef FILEMONITOR_H
#define FILEMONITOR_H

#include <QObject>

class QFileSystemWatcher;
class QTimer;

class FileMonitor : public QObject {
  Q_OBJECT

 public:
  explicit FileMonitor(const QString &filePath, QObject *parent = nullptr);
  void setLastKnownContent(const QString &content);
  void setActive(bool active);

 signals:
  void changedExternally(const QString &content);

 private:
  QFileSystemWatcher *watcher;
  QString path;
  QString lastContent;
  QTimer *debounce;
  bool active = false;

  void rearm();
  void check();
};

#endif  // FILEMONITOR_H
