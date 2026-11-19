#include "config.h"
#include "mainwindow.h"
#include "utils.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QLocale>
#include <QMessageBox>
#include <QTranslator>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  QApplication::setApplicationName("QGoForIt");
  QApplication::setApplicationVersion(QString(APP_VERSION));

  // The startup error dialogs must be translated, so the translator is
  // installed before any of them can show. translationsDir() does not
  // depend on configDir()/dataDir().
  QTranslator *translator = new QTranslator(&a);
  const QString baseName = "qgoforit_" + QLocale::system().name();
  if (translator->load(baseName, Paths::translationsDir())) {
    a.installTranslator(translator);
  }

  if (!QDir().mkpath(Paths::configDir()) ||
      !QDir().mkpath(Paths::dataDir())) {
    QMessageBox::critical(nullptr, "QGoForIt",
                          QObject::tr("Failed to create the configuration "
                                      "and data directories."));
    return 1;
  }

  // Lock before touching any data files: a second instance must not
  // create or open todo.txt/done.txt.
  QLockFile lockFile(Paths::dataDir() + "/QGoForIt.lock");
  lockFile.setStaleLockTime(0);
  if (!lockFile.tryLock(100)) {
    QMessageBox::information(nullptr, "QGoForIt",
                              QObject::tr("QGoForIt is already running."),
                              QMessageBox::Ok);
    return 0;
  }

  for (const auto &path : {Paths::todoFile(), Paths::doneFile()}) {
    if (QFile::exists(path)) continue;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
      QMessageBox::critical(nullptr, "QGoForIt",
                            QObject::tr("Failed to create %1: %2")
                                .arg(path, f.errorString()));
      return 1;
    }
    f.close();
  }

  MainWindow w;
  w.show();
  return a.exec();
}
