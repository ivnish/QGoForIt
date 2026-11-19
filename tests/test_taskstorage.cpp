#include "todowidget.h"
#include "utils.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include <unistd.h>

namespace {
bool runningAsRoot() { return geteuid() == 0; }

void closePopupSoon() {
  // The storage-failure warning boxes are modal; schedule a close from
  // the event loop so failure paths can run unattended.
  QTimer::singleShot(0, qApp, []() {
    QWidget *popup = QApplication::activeModalWidget();
    if (popup) popup->close();
  });
}
}  // namespace

class TestTaskStorage : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();
  void completionFailureKeepsTask();
  void completionFailureSingleWarning();
  void completionRollsBackAppendWhenRewriteFails();
  void completionSuccessMovesTask();
  void uncompletionFailureKeepsTask();
  void uncompletionRollsBackAppendWhenRewriteFails();
  void addNewTaskRollsBackOnSaveFailure();
  void deleteRollsBackOnSaveFailure();
  void deleteSuccessRemovesTask();
  void reloadFailureKeepsState();
  void reloadDirectoryKeepsState();
  void doneReloadFailureKeepsState();
  void reloadDuringDragCancelsIt();
  void noOpEditDoesNotRewriteFile();
  void reloadWithOpenEditorNoSave();
  void renameUpdatesActiveTask();

 private:
  QTemporaryDir dir;
};

void TestTaskStorage::initTestCase() {
  // Redirect QSettings away from the developer's real config.
  QStandardPaths::setTestModeEnabled(true);
  QVERIFY(dir.isValid());
}

void TestTaskStorage::completionFailureKeepsTask() {
  if (runningAsRoot()) {
    QSKIP("Permission-based failure simulation cannot run as root.");
  }

  const QString todoPath = dir.filePath("todo_fail.txt");
  const QString donePath = dir.filePath("done_fail.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Write report\n"));
  QVERIFY(FileUtils::writeFile(donePath, ""));
  QVERIFY(QFile::setPermissions(donePath, QFile::ReadOwner));

  TodoWidget todo(todoPath, donePath);
  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 1);

  closePopupSoon();
  rows.first()->setChecked(true);

  const auto todoContent = FileUtils::readFile(todoPath);
  QVERIFY(todoContent.has_value());
  QCOMPARE(todoContent->trimmed(), "Write report");

  const auto rowsAfter = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rowsAfter.size(), 1);
  QVERIFY(!rowsAfter.first()->isChecked());
}

void TestTaskStorage::completionRollsBackAppendWhenRewriteFails() {
  if (runningAsRoot()) {
    QSKIP("Permission-based failure simulation cannot run as root.");
  }

  const QString todoPath = dir.filePath("todo_rb.txt");
  const QString donePath = dir.filePath("done_rb.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Write report\n"));
  QVERIFY(FileUtils::writeFile(donePath, ""));

  // done.txt stays writable so the append succeeds, while the read-only
  // directory makes the QSaveFile rewrite of todo.txt fail.
  const QString dirPath = dir.path();
  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::ExeOwner));

  TodoWidget todo(todoPath, donePath);
  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 1);

  closePopupSoon();
  rows.first()->setChecked(true);

  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::WriteOwner |
                                              QFile::ExeOwner));

  // The append was rolled back: no duplicate in done.txt.
  const auto doneContent = FileUtils::readFile(donePath);
  QVERIFY(doneContent.has_value());
  QCOMPARE(doneContent->trimmed(), QString(""));

  // The task is still stored in todo.txt and visible, unchecked.
  const auto todoContent = FileUtils::readFile(todoPath);
  QVERIFY(todoContent.has_value());
  QCOMPARE(todoContent->trimmed(), "Write report");
  const auto rowsAfter = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rowsAfter.size(), 1);
  QVERIFY(!rowsAfter.first()->isChecked());
}

void TestTaskStorage::completionFailureSingleWarning() {
  if (runningAsRoot()) {
    QSKIP("Permission-based failure simulation cannot run as root.");
  }

  const QString todoPath = dir.filePath("todo_warn.txt");
  const QString donePath = dir.filePath("done_warn.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Write report\n"));
  QVERIFY(FileUtils::writeFile(donePath, ""));
  QVERIFY(QFile::setPermissions(todoPath, QFile::ReadOwner));
  QVERIFY(QFile::setPermissions(donePath, QFile::ReadOwner));

  // A read-only directory makes the QSaveFile-based rewrite of
  // todo.txt fail as well. The checkbox revert must not attempt it:
  // exactly one warning (for done.txt) must be shown.
  const QString dirPath = dir.path();
  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::ExeOwner));

  TodoWidget todo(todoPath, donePath);
  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 1);

  int closedModals = 0;
  QTimer closer;
  closer.setInterval(10);
  QObject::connect(&closer, &QTimer::timeout, qApp, [&closedModals]() {
    QWidget *popup = QApplication::activeModalWidget();
    if (popup) {
      ++closedModals;
      popup->close();
    }
  });
  closer.start();
  rows.first()->setChecked(true);
  closer.stop();

  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::WriteOwner |
                                              QFile::ExeOwner));
  QVERIFY(QFile::setPermissions(todoPath,
                                QFile::ReadOwner | QFile::WriteOwner));
  QVERIFY(QFile::setPermissions(donePath,
                                QFile::ReadOwner | QFile::WriteOwner));

  QCOMPARE(closedModals, 1);

  const auto rowsAfter = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rowsAfter.size(), 1);
  QVERIFY(!rowsAfter.first()->isChecked());
}

void TestTaskStorage::completionSuccessMovesTask() {
  const QString todoPath = dir.filePath("todo_ok.txt");
  const QString donePath = dir.filePath("done_ok.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Buy milk\n"));
  QVERIFY(FileUtils::writeFile(donePath, ""));

  TodoWidget todo(todoPath, donePath);
  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 1);

  rows.first()->setChecked(true);

  const auto doneContent = FileUtils::readFile(donePath);
  QVERIFY(doneContent.has_value());
  QVERIFY(doneContent->contains("Buy milk"));
  QVERIFY(doneContent->contains(DateUtils::currentDateStr()));

  const auto todoContent = FileUtils::readFile(todoPath);
  QVERIFY(todoContent.has_value());
  QCOMPARE(todoContent->trimmed(), QString(""));

  QTRY_COMPARE(todo.findChildren<TaskRowWidget *>().size(), 0);
}

void TestTaskStorage::uncompletionFailureKeepsTask() {
  if (runningAsRoot()) {
    QSKIP("Permission-based failure simulation cannot run as root.");
  }

  const QString todoPath = dir.filePath("todo_unfail.txt");
  const QString donePath = dir.filePath("done_unfail.txt");
  const QString line = DateUtils::currentDateStr() + " Old task";
  QVERIFY(FileUtils::writeFile(donePath, line + "\n"));
  QVERIFY(FileUtils::writeFile(todoPath, ""));
  QVERIFY(QFile::setPermissions(todoPath, QFile::ReadOwner));

  DoneWidget done(donePath, todoPath);
  const auto rows = done.findChildren<DoneRowWidget *>();
  QCOMPARE(rows.size(), 1);

  closePopupSoon();
  rows.first()->setChecked(false);

  const auto doneContent = FileUtils::readFile(donePath);
  QVERIFY(doneContent.has_value());
  QCOMPARE(doneContent->trimmed(), line);

  const auto rowsAfter = done.findChildren<DoneRowWidget *>();
  QCOMPARE(rowsAfter.size(), 1);
  QVERIFY(rowsAfter.first()->isChecked());
}

void TestTaskStorage::uncompletionRollsBackAppendWhenRewriteFails() {
  if (runningAsRoot()) {
    QSKIP("Permission-based failure simulation cannot run as root.");
  }

  const QString todoPath = dir.filePath("todo_unrb.txt");
  const QString donePath = dir.filePath("done_unrb.txt");
  const QString line = DateUtils::currentDateStr() + " Old task";
  QVERIFY(FileUtils::writeFile(donePath, line + "\n"));
  QVERIFY(FileUtils::writeFile(todoPath, ""));

  // todo.txt stays writable so the append succeeds, while the read-only
  // directory makes the QSaveFile rewrite of done.txt fail.
  const QString dirPath = dir.path();
  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::ExeOwner));

  DoneWidget done(donePath, todoPath);
  const auto rows = done.findChildren<DoneRowWidget *>();
  QCOMPARE(rows.size(), 1);

  closePopupSoon();
  rows.first()->setChecked(false);

  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::WriteOwner |
                                              QFile::ExeOwner));

  // The append was rolled back: todo.txt is empty again.
  const auto todoContent = FileUtils::readFile(todoPath);
  QVERIFY(todoContent.has_value());
  QCOMPARE(todoContent->trimmed(), QString(""));

  // The task is still stored in done.txt and visible, checked.
  const auto doneContent = FileUtils::readFile(donePath);
  QVERIFY(doneContent.has_value());
  QCOMPARE(doneContent->trimmed(), line);
  const auto rowsAfter = done.findChildren<DoneRowWidget *>();
  QCOMPARE(rowsAfter.size(), 1);
  QVERIFY(rowsAfter.first()->isChecked());
}

void TestTaskStorage::addNewTaskRollsBackOnSaveFailure() {
  if (runningAsRoot()) {
    QSKIP("Permission-based failure simulation cannot run as root.");
  }

  const QString todoPath = dir.filePath("todo_add.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Existing\n"));
  const QString dirPath = dir.path();
  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::ExeOwner));

  TodoWidget todo(todoPath, dir.filePath("done_add.txt"));
  auto *input = todo.findChild<QLineEdit *>();
  QVERIFY(input);

  closePopupSoon();
  input->setText("New task");
  QTest::keyClick(input, Qt::Key_Return);

  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::WriteOwner |
                                              QFile::ExeOwner));

  // The phantom row is removed (deleteLater needs an event loop).
  QTRY_COMPARE(todo.findChildren<TaskRowWidget *>().size(), 1);
  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.first()->text(), "Existing");
  QCOMPARE(input->text(), "New task");

  const auto content = FileUtils::readFile(todoPath);
  QVERIFY(content.has_value());
  QCOMPARE(content->trimmed(), "Existing");
}

void TestTaskStorage::deleteRollsBackOnSaveFailure() {
  if (runningAsRoot()) {
    QSKIP("Permission-based failure simulation cannot run as root.");
  }

  const QString todoPath = dir.filePath("todo_del.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Task A\n"));
  const QString dirPath = dir.path();
  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::ExeOwner));

  TodoWidget todo(todoPath, dir.filePath("done_del.txt"));
  todo.resize(400, 300);
  todo.show();
  QApplication::processEvents();

  auto *list = todo.findChild<TodoListWidget *>();
  QVERIFY(list);
  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 1);

  QSignalSpy spy(&todo, &TodoWidget::activeTaskChanged);
  QTest::mouseClick(rows.first(), Qt::LeftButton, Qt::NoModifier,
                    QPoint(40, 10));
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).value(0).toString(), "Task A");

  // Any save attempt now fails with a modal warning; close them so the
  // two delete attempts can run unattended.
  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::ExeOwner));

  int closedModals = 0;
  QTimer closer;
  closer.setInterval(10);
  QObject::connect(&closer, &QTimer::timeout, qApp, [&closedModals]() {
    QWidget *popup = QApplication::activeModalWidget();
    if (popup) {
      ++closedModals;
      popup->close();
    }
  });
  closer.start();
  QTest::keyClick(list, Qt::Key_Delete);
  // The row stayed selected, so Delete must work on it again.
  QTest::keyClick(list, Qt::Key_Delete);
  closer.stop();

  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::WriteOwner |
                                              QFile::ExeOwner));

  // Both deletions failed to persist: the row is back in the list.
  QCOMPARE(closedModals, 2);
  const auto rowsAfter = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rowsAfter.size(), 1);
  QCOMPARE(rowsAfter.first()->text(), "Task A");
  // The selection was never cleared, so no activeTaskChanged("") was
  // emitted at any point.
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).value(0).toString(), "Task A");

  const auto content = FileUtils::readFile(todoPath);
  QVERIFY(content.has_value());
  QCOMPARE(content->trimmed(), "Task A");
}

void TestTaskStorage::deleteSuccessRemovesTask() {
  const QString todoPath = dir.filePath("todo_delok.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Task A\nTask B\n"));

  TodoWidget todo(todoPath, dir.filePath("done_delok.txt"));
  todo.resize(400, 300);
  todo.show();
  QApplication::processEvents();

  auto *list = todo.findChild<TodoListWidget *>();
  QVERIFY(list);
  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 2);

  QTest::mouseClick(rows.first(), Qt::LeftButton, Qt::NoModifier,
                    QPoint(40, 10));
  QTest::keyClick(list, Qt::Key_Delete);

  QTRY_COMPARE(todo.findChildren<TaskRowWidget *>().size(), 1);
  const auto after = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(after.first()->text(), "Task B");

  const auto content = FileUtils::readFile(todoPath);
  QVERIFY(content.has_value());
  QCOMPARE(content->trimmed(), "Task B");
}

void TestTaskStorage::reloadFailureKeepsState() {
  const QString todoPath = dir.filePath("todo_reload.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Persisted task\n"));

  TodoWidget todo(todoPath, dir.filePath("done_reload.txt"));
  QCOMPARE(todo.findChildren<TaskRowWidget *>().size(), 1);

  // A transient read error (missing file) must not blank the UI.
  QVERIFY(QFile::remove(todoPath));
  todo.reload();

  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 1);
  QCOMPARE(rows.first()->text(), "Persisted task");
}

void TestTaskStorage::reloadDirectoryKeepsState() {
  const QString todoPath = dir.filePath("todo_dirswap.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Persisted task\n"));

  TodoWidget todo(todoPath, dir.filePath("done_dirswap.txt"));
  QCOMPARE(todo.findChildren<TaskRowWidget *>().size(), 1);

  // The file turns into a directory: on Linux open() succeeds and the
  // read fails — this must not be treated as an empty file.
  QVERIFY(QFile::remove(todoPath));
  QVERIFY(QDir().mkpath(todoPath));
  todo.reload();

  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 1);
  QCOMPARE(rows.first()->text(), "Persisted task");
}

void TestTaskStorage::doneReloadFailureKeepsState() {
  const QString donePath = dir.filePath("done_reload.txt");
  const QString line = DateUtils::currentDateStr() + " Kept task";
  QVERIFY(FileUtils::writeFile(donePath, line + "\n"));

  DoneWidget done(donePath, dir.filePath("todo_reload.txt"));
  QCOMPARE(done.findChildren<DoneRowWidget *>().size(), 1);

  // A transient read error (missing file) must not blank the list.
  QVERIFY(QFile::remove(donePath));
  done.reload();

  const auto rows = done.findChildren<DoneRowWidget *>();
  QCOMPARE(rows.size(), 1);
  QCOMPARE(rows.first()->originalLine(), line);
}

void TestTaskStorage::reloadDuringDragCancelsIt() {
  const QString todoPath = dir.filePath("todo_drag.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Task A\nTask B\n"));

  TodoWidget todo(todoPath, dir.filePath("done_drag.txt"));
  todo.resize(400, 300);
  todo.show();
  QApplication::processEvents();

  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 2);
  auto *row = rows.first();

  // The drag handle is the rightmost child label.
  QLabel *dragHandle = nullptr;
  const auto labels = row->findChildren<QLabel *>();
  for (QLabel *l : labels) {
    if (!dragHandle || l->geometry().x() > dragHandle->geometry().x()) {
      dragHandle = l;
    }
  }
  QVERIFY(dragHandle);
  const QPoint handleCenter = dragHandle->geometry().center();

  // Direct delivery: reliable under the offscreen platform.
  QMouseEvent press(QEvent::MouseButtonPress, handleCenter,
                    row->mapToGlobal(handleCenter), Qt::LeftButton,
                    Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(row, &press);
  const QPoint movePos = handleCenter + QPoint(12, 0);
  QMouseEvent move(QEvent::MouseMove, movePos,
                   row->mapToGlobal(movePos), Qt::LeftButton,
                   Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(row, &move);

  // The drag is active (row detached, timer armed); an external reload
  // must cancel it instead of reinserting a stale row.
  todo.reload();

  auto *list = todo.findChild<TodoListWidget *>();
  QVERIFY(list);
  QTest::mouseRelease(list, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
  QTest::qWait(100);

  QTRY_COMPARE(todo.findChildren<TaskRowWidget *>().size(), 2);
  int aCount = 0;
  int bCount = 0;
  const auto rowsAfter = todo.findChildren<TaskRowWidget *>();
  for (TaskRowWidget *r : rowsAfter) {
    if (r->text() == "Task A") ++aCount;
    else if (r->text() == "Task B") ++bCount;
  }
  QCOMPARE(aCount, 1);
  QCOMPARE(bCount, 1);

  const auto content = FileUtils::readFile(todoPath);
  QVERIFY(content.has_value());
  QCOMPARE(content->trimmed(), "Task A\nTask B");
}

void TestTaskStorage::noOpEditDoesNotRewriteFile() {
  const QString todoPath = dir.filePath("todo_noop.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Task A\n"));

  TodoWidget todo(todoPath, dir.filePath("done_noop.txt"));
  todo.resize(400, 300);
  todo.show();
  QApplication::processEvents();

  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 1);
  // Fresh QFileInfo instances: QFileInfo caches stat results.
  const qint64 mtimeBefore =
      QFileInfo(todoPath).lastModified().toMSecsSinceEpoch();

  // Open the editor and commit without any change.
  QTest::mouseDClick(rows.first(), Qt::LeftButton, Qt::NoModifier,
                     QPoint(40, 10));
  const auto editors = rows.first()->findChildren<QLineEdit *>();
  QCOMPARE(editors.size(), 1);
  QTest::keyClick(editors.first(), Qt::Key_Return);

  const qint64 mtimeAfter =
      QFileInfo(todoPath).lastModified().toMSecsSinceEpoch();
  QCOMPARE(mtimeAfter, mtimeBefore);
}

void TestTaskStorage::reloadWithOpenEditorNoSave() {
  if (runningAsRoot()) {
    QSKIP("Permission-based failure simulation cannot run as root.");
  }

  const QString todoPath = dir.filePath("todo_late.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Task A\n"));

  TodoWidget todo(todoPath, dir.filePath("done_late.txt"));
  todo.resize(400, 300);
  todo.show();
  QApplication::processEvents();

  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 1);
  QTest::mouseDClick(rows.first(), Qt::LeftButton, Qt::NoModifier,
                     QPoint(40, 10));
  QVERIFY(!rows.first()->findChildren<QLineEdit *>().isEmpty());

  todo.reload();

  // The old row is alive until deleteLater() runs, so its editor is
  // still the only line edit. Committing a rename from it must not
  // fire a save (a no-op commit emits nothing since the renamed
  // gating, so the text must actually change).
  auto *dyingEditor = todo.findChild<QLineEdit *>();
  QVERIFY(dyingEditor);
  QTest::keyClicks(dyingEditor, "Edited");

  // Any save attempt now fails with a modal warning.
  const QString dirPath = dir.path();
  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::ExeOwner));

  int closedModals = 0;
  QTimer closer;
  closer.setInterval(10);
  QObject::connect(&closer, &QTimer::timeout, qApp, [&closedModals]() {
    QWidget *popup = QApplication::activeModalWidget();
    if (popup) {
      ++closedModals;
      popup->close();
    }
  });
  closer.start();
  QTest::keyClick(dyingEditor, Qt::Key_Return);
  QTest::qWait(30);
  closer.stop();

  QVERIFY(QFile::setPermissions(dirPath, QFile::ReadOwner | QFile::WriteOwner |
                                              QFile::ExeOwner));

  QCOMPARE(closedModals, 0);
  QCOMPARE(todo.findChildren<TaskRowWidget *>().size(), 1);
  QCOMPARE(todo.findChildren<TaskRowWidget *>().first()->text(), "Task A");
}

void TestTaskStorage::renameUpdatesActiveTask() {
  const QString todoPath = dir.filePath("todo_rename.txt");
  QVERIFY(FileUtils::writeFile(todoPath, "Task A\n"));

  TodoWidget todo(todoPath, dir.filePath("done_rename.txt"));
  todo.resize(400, 300);
  todo.show();
  QApplication::processEvents();

  const auto rows = todo.findChildren<TaskRowWidget *>();
  QCOMPARE(rows.size(), 1);
  auto *row = rows.first();

  QSignalSpy spy(&todo, &TodoWidget::activeTaskChanged);
  QVERIFY(spy.isValid());

  // Click on the label area (the drag handle sits at the right edge).
  QTest::mouseClick(row, Qt::LeftButton, Qt::NoModifier, QPoint(40, 10));
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).value(0).toString(), "Task A");

  // Inline rename of the selected task.
  QTest::mouseDClick(row, Qt::LeftButton, Qt::NoModifier, QPoint(40, 10));
  const auto editors = row->findChildren<QLineEdit *>();
  QCOMPARE(editors.size(), 1);
  QTest::keyClicks(editors.first(), "Task B");
  QTest::keyClick(editors.first(), Qt::Key_Return);

  QCOMPARE(spy.count(), 2);
  QCOMPARE(spy.at(1).value(0).toString(), "Task B");

  const auto todoContent = FileUtils::readFile(todoPath);
  QVERIFY(todoContent.has_value());
  QCOMPARE(todoContent->trimmed(), "Task B");
}

QTEST_MAIN(TestTaskStorage)
#include "test_taskstorage.moc"
