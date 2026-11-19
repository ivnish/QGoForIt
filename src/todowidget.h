#ifndef TODOWIDGET_H
#define TODOWIDGET_H

#include "baserowwidget.h"
#include "utils.h"

#include <QScrollArea>
#include <QWidget>

class QLabel;
class QLineEdit;
class QVBoxLayout;
class QTimer;

class TaskRowWidget : public BaseRowWidget {
  Q_OBJECT

 public:
  explicit TaskRowWidget(const QString &text, bool checked = false,
                         QWidget *parent = nullptr);

  QString text() const { return labelText; }
  bool isChecked() const;
  void setChecked(bool c);
  void setCheckedSilently(bool c);
  void startEditing();
  void applyHighlight(const QColor &bgColor, const QColor &textColor);
  void removeHighlight(const QColor &defaultBgColor);
  void retheme(const Theme::Palette &t);
  QLabel *dragHandleWidget() { return dragHandle; }

 signals:
  void dragStarted(TaskRowWidget *row);
  void clicked(TaskRowWidget *row);
  void checkedChanged();
  void taskCompleted(TaskRowWidget *row);
  void textChanged(const QString &text);

 protected:
  void mousePressEvent(QMouseEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;

 private:
  QLabel *dragHandle;
  QLineEdit *editor = nullptr;
  QString labelText;
  QPoint pressPos;
  bool pressedOnHandle = false;

  void commitEdit();
  void cancelEdit();
};

class TodoListWidget : public QScrollArea {
  Q_OBJECT

 public:
  explicit TodoListWidget(QWidget *parent = nullptr);
  TaskRowWidget *addTask(const QString &text, bool checked = false);
  void removeRow(TaskRowWidget *row);
  void clearTasks();
  // Temporarily detach a row (kept alive) so serialization skips it;
  // returns its layout index for attachRow().
  int detachRow(TaskRowWidget *row);
  void attachRow(TaskRowWidget *row, int index);
  QList<QPair<QString, bool>> tasks() const;
  void deselectAll();
  void adjustRowHeights();
  void retheme(const Theme::Palette &t);

 signals:
  void contentChanged();
  void taskCompleted(TaskRowWidget *row);
  void taskDeleteRequested(TaskRowWidget *row);
  void activeTaskChanged(const QString &text);

 protected:
  void keyPressEvent(QKeyEvent *e) override;

 private:
  QWidget *container;
  QVBoxLayout *layout;
  QWidget *dropGap;
  QLabel *dragGhost = nullptr;
  TaskRowWidget *draggedRow = nullptr;
  TaskRowWidget *selectedRow = nullptr;
  int dragOriginalIndex = -1;
  QPoint dragHandleOffset;
  QTimer *dragTimer = nullptr;
  QTimer *scrollTimer = nullptr;
  int scrollDirection = 0;

  void startDrag(TaskRowWidget *row);
  void dragTick();
  void finishDrag();
  void updateDropIndicator(const QPoint &viewportPos);
  int computeDropRow(const QPoint &viewportPos) const;
  void moveGapToRow(int rowCount);
  int gapRowCount() const;
  void updateDropGapColor(const Theme::Palette &t);
  void checkAutoScroll(int viewportY);
  void selectRow(TaskRowWidget *row);
  void cancelDrag();
};

class QToolButton;
class FileMonitor;

class TodoWidget : public QWidget {
  Q_OBJECT

 public:
  explicit TodoWidget(const QString &todoPath, const QString &donePath,
                      QWidget *parent = nullptr);
  void reload();
  void setWatcherEnabled(bool enabled);
  void retheme(const Theme::Palette &t);

 signals:
  void doneListChanged();
  void activeTaskChanged(const QString &text);

 private:
  TodoListWidget *listWidget;
  QWidget *inputContainer;
  QLineEdit *taskInput;
  QToolButton *addButton;
  FileMonitor *fileMonitor;
  QString todoFilePath;
  QString doneFilePath;
  void loadFromFile();
  void populateTasks(const QString &content);
  bool saveToFile();
  void addNewTask();
  void onTaskCompleted(TaskRowWidget *row);
  void onTaskDeleteRequested(TaskRowWidget *row);
};

class DoneRowWidget : public BaseRowWidget {
  Q_OBJECT

 public:
  explicit DoneRowWidget(const QString &originalLine, QWidget *parent = nullptr);
  QString originalLine() const { return fullLine; }
  bool isChecked() const;
  void setChecked(bool c);
  void setCheckedSilently(bool c);
  void retheme(const Theme::Palette &t);

 signals:
  void taskUncompleted(const QString &text);

 private:
  QString fullLine;
  QString taskText;
};

class DoneWidget : public QWidget {
  Q_OBJECT

 public:
  explicit DoneWidget(const QString &donePath, const QString &todoPath,
                      QWidget *parent = nullptr);
  void reload();
  void adjustRowHeights();
  void setAutoCleanup(bool enabled, int days);
  void retheme(const Theme::Palette &t);

 signals:
  void todoListChanged();

 private:
  QScrollArea *scrollArea;
  QWidget *container;
  QVBoxLayout *layout;
  QString doneFilePath;
  QString todoFilePath;
  int cleanupDays = 0;
  void loadFromFile();
  void populateTasks(const QString &content);
  bool saveToFile();
  void clearTasks();
  void onTaskUncompleted(const QString &text);
  void cleanupOldTasks();
};

#endif  // TODOWIDGET_H
