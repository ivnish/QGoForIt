#include "utils.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <unistd.h>

namespace {
bool runningAsRoot() { return geteuid() == 0; }
}  // namespace

class TestFileUtils : public QObject {
  Q_OBJECT

 private slots:
  void writeReadRoundtrip();
  void appendRoundtrip();
  void writeFailureKeepsOldContent();
  void appendFailureKeepsContent();
  void readDirectoryReturnsNoValue();

 private:
  QTemporaryDir dir;
};

void TestFileUtils::writeReadRoundtrip() {
  const QString path = dir.filePath("roundtrip.txt");
  QVERIFY(FileUtils::writeFile(path, "first line\nsecond line\n"));

  const auto content = FileUtils::readFile(path);
  QVERIFY(content.has_value());
  QCOMPARE(*content, "first line\nsecond line\n");
}

void TestFileUtils::appendRoundtrip() {
  const QString path = dir.filePath("append_roundtrip.txt");
  QVERIFY(FileUtils::writeFile(path, "one\n"));
  QVERIFY(FileUtils::appendFile(path, "two\n"));

  const auto content = FileUtils::readFile(path);
  QVERIFY(content.has_value());
  QCOMPARE(*content, "one\ntwo\n");
}

void TestFileUtils::writeFailureKeepsOldContent() {
  if (runningAsRoot()) {
    QSKIP("Permission-based failure simulation cannot run as root.");
  }

  const QString path = dir.filePath("atomic.txt");
  QVERIFY(FileUtils::writeFile(path, "original\n"));

  // A read-only directory makes QSaveFile fail when creating the
  // temporary file, simulating an I/O failure before the commit.
  const QString dirPath = dir.path();
  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::ExeOwner));
  const bool saved = FileUtils::writeFile(path, "replaced\n");
  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner |
                                              QFile::WriteOwner |
                                              QFile::ExeOwner));

  QVERIFY(!saved);
  const auto content = FileUtils::readFile(path);
  QVERIFY(content.has_value());
  QCOMPARE(*content, "original\n");
}

void TestFileUtils::appendFailureKeepsContent() {
  if (runningAsRoot()) {
    QSKIP("Permission-based failure simulation cannot run as root.");
  }

  const QString path = dir.filePath("append_fail.txt");
  QVERIFY(FileUtils::writeFile(path, "kept\n"));
  QVERIFY(QFile::setPermissions(path, QFile::ReadOwner));

  const bool appended = FileUtils::appendFile(path, "dropped\n");
  QVERIFY(QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner));

  QVERIFY(!appended);
  const auto content = FileUtils::readFile(path);
  QVERIFY(content.has_value());
  QCOMPARE(*content, "kept\n");
}

void TestFileUtils::readDirectoryReturnsNoValue() {
  // Opening a directory succeeds on Linux, but the read fails: an
  // empty QString must not be mistaken for a valid empty file.
  const QString subDir = dir.filePath("read_dir");
  QVERIFY(QDir().mkpath(subDir));
  QVERIFY(!FileUtils::readFile(subDir).has_value());
}

QTEST_MAIN(TestFileUtils)
#include "test_fileutils.moc"
