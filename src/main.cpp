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

  QDir().mkpath(Paths::configDir());
  QDir().mkpath(Paths::dataDir());

  for (const auto &path : {Paths::todoFile(), Paths::doneFile()}) {
    if (!QFile::exists(path)) {
      QFile f(path);
      f.open(QIODevice::WriteOnly);
      f.close();
    }
  }

  QLockFile lockFile(Paths::dataDir() + "/QGoForIt.lock");
  lockFile.setStaleLockTime(0);
  if (!lockFile.tryLock(100)) {
    QMessageBox::information(nullptr, "QGoForIt",
                              QObject::tr("QGoForIt is already running."),
                              QMessageBox::Ok);
    return 0;
  }

  QTranslator *translator = new QTranslator(&a);
  const QString baseName = "qgoforit_" + QLocale::system().name();
  if (translator->load(baseName, Paths::translationsDir())) {
    a.installTranslator(translator);
  }

  MainWindow w;
  w.show();
  return a.exec();
}
