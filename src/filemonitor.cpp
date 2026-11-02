#include "filemonitor.h"

#include "constants.h"
#include "utils.h"

#include <QFileSystemWatcher>
#include <QTimer>

FileMonitor::FileMonitor(const QString &filePath, QObject *parent)
    : QObject(parent), path(filePath) {
  watcher = new QFileSystemWatcher(this);
  connect(watcher, &QFileSystemWatcher::fileChanged, this,
          [this]() {
            rearm();
            debounce->start();
          });

  debounce = new QTimer(this);
  debounce->setSingleShot(true);
  debounce->setInterval(Sizes::WATCHER_DEBOUNCE_MS);
  connect(debounce, &QTimer::timeout, this, &FileMonitor::check);
}

void FileMonitor::setLastKnownContent(const QString &content) {
  lastContent = content;
}

void FileMonitor::setActive(bool on) {
  if (active == on) return;
  active = on;
  if (active) {
    watcher->addPath(path);
  } else {
    watcher->removePath(path);
    debounce->stop();
  }
}

void FileMonitor::rearm() {
  if (!active) return;
  if (!watcher->files().contains(path)) {
    watcher->addPath(path);
  }
}

void FileMonitor::check() {
  if (!active) return;
  rearm();

  const auto content = FileUtils::readFile(path);
  if (!content) return;

  if (*content != lastContent) {
    lastContent = *content;
    emit changedExternally(*content);
  }
}
