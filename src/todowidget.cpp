#include "todowidget.h"

#include "constants.h"
#include "filemonitor.h"
#include "utils.h"

#include <QApplication>
#include <QCheckBox>
#include <QCursor>
#include <QDate>
#include <QDebug>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

TaskRowWidget::TaskRowWidget(const QString &text, bool checked, QWidget *parent)
    : BaseRowWidget(parent), labelText(text) {
  checkbox->setStyleSheet(Styles::CHECKBOX_TASK);
  checkbox->setChecked(checked);
  connect(checkbox, &QCheckBox::toggled, this, [this](bool c) {
    if (c) {
      emit taskCompleted(labelText);
    } else {
      emit checkedChanged();
    }
  });

  label->setText(text);
  label->setObjectName("taskLabel");

  dragHandle = new QLabel(QStringLiteral("\u22ee"), this);
  dragHandle->setObjectName("dragHandle");
  dragHandle->setFixedWidth(Sizes::DRAG_HANDLE_WIDTH);
  dragHandle->setAlignment(Qt::AlignCenter);
  dragHandle->setStyleSheet(
      QString("color: %1; font-size: %2px;")
          .arg(Colors::DRAG_HANDLE)
          .arg(Sizes::DRAG_HANDLE_FONT_SIZE));
  dragHandle->setCursor(Qt::OpenHandCursor);

  hLayout->addWidget(dragHandle);
}

bool TaskRowWidget::isChecked() const { return checkbox->isChecked(); }
void TaskRowWidget::setChecked(bool c) { checkbox->setChecked(c); }

void TaskRowWidget::commitEdit() {
  labelText = editor->text().trimmed();
  if (labelText.isEmpty()) labelText = label->text();
  label->setText(labelText);
  editor->deleteLater();
  editor = nullptr;
  checkbox->show();
  dragHandle->show();
  label->show();
  QTimer::singleShot(0, this, [this]() { adjustHeight(); });
  emit checkedChanged();
}

void TaskRowWidget::cancelEdit() {
  if (!editor) return;
  editor->deleteLater();
  editor = nullptr;
  checkbox->show();
  dragHandle->show();
  label->show();
}

void TaskRowWidget::startEditing() {
  if (editor) return;

  editor = new QLineEdit(labelText, this);
  QFont f = label->font();
  editor->setFont(f);
  editor->setFrame(false);
  editor->setStyleSheet("background: white; color: black; padding: 0 2px;");
  label->hide();
  checkbox->hide();
  dragHandle->hide();
  hLayout->addWidget(editor);
  editor->setFocus();
  editor->selectAll();

  connect(editor, &QLineEdit::returnPressed, this, &TaskRowWidget::commitEdit);
  connect(editor, &QLineEdit::editingFinished, this, [this]() {
    if (editor) commitEdit();
  });
  auto *shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), editor);
  connect(shortcut, &QShortcut::activated, this, &TaskRowWidget::cancelEdit);
}

void TaskRowWidget::applyHighlight(const QColor &bgColor,
                                    const QColor &textColor) {
  QPalette pal = palette();
  pal.setColor(QPalette::Window, bgColor);
  setAutoFillBackground(true);
  setPalette(pal);
  label->setStyleSheet(
      QString("color: %1; background: transparent;").arg(textColor.name()));
}

void TaskRowWidget::removeHighlight(const QColor &defaultBgColor) {
  QPalette pal = palette();
  pal.setColor(QPalette::Window, defaultBgColor);
  setAutoFillBackground(false);
  setPalette(pal);
  label->setStyleSheet("");
}

void TaskRowWidget::mouseDoubleClickEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    startEditing();
    e->accept();
    return;
  }
  QWidget::mouseDoubleClickEvent(e);
}

void TaskRowWidget::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton &&
      dragHandle->geometry().contains(e->pos())) {
    pressedOnHandle = true;
    pressPos = e->pos();
    e->accept();
    return;
  }
  if (e->button() == Qt::LeftButton) {
    emit clicked(this);
  }
  QWidget::mousePressEvent(e);
}

void TaskRowWidget::mouseMoveEvent(QMouseEvent *e) {
  if (pressedOnHandle && (e->buttons() & Qt::LeftButton)) {
    if ((e->pos() - pressPos).manhattanLength() > 5) {
      pressedOnHandle = false;
      emit dragStarted(this);
      return;
    }
  }
  QWidget::mouseMoveEvent(e);
}

void TaskRowWidget::mouseReleaseEvent(QMouseEvent *e) {
  pressedOnHandle = false;
  QWidget::mouseReleaseEvent(e);
}

TodoListWidget::TodoListWidget(QWidget *parent) : QScrollArea(parent) {
  setFocusPolicy(Qt::StrongFocus);

  container = UiUtils::setupListScrollArea(this);

  layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  dropGap = new QWidget(container);
  dropGap->setFixedHeight(0);
  dropGap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  dropGap->setAutoFillBackground(true);
  QPalette gapPalette = dropGap->palette();
  gapPalette.setColor(QPalette::Window, QColor(26, 130, 220, 55));
  dropGap->setPalette(gapPalette);

  layout->addWidget(dropGap);
  layout->addStretch(1);

  scrollTimer = new QTimer(this);
  scrollTimer->setInterval(Sizes::SCROLL_INTERVAL);
  connect(scrollTimer, &QTimer::timeout, this, [this]() {
    if (scrollDirection == 0) return;
    verticalScrollBar()->setValue(verticalScrollBar()->value() +
                                   Sizes::SCROLL_STEP * scrollDirection);
    updateDropIndicator(viewport()->mapFromGlobal(QCursor::pos()));
  });
}

void TodoListWidget::addTask(const QString &text, bool checked) {
  auto *row = new TaskRowWidget(text, checked, container);
  connect(row, &TaskRowWidget::dragStarted, this,
          &TodoListWidget::startDrag);
  connect(row, &TaskRowWidget::clicked, this, &TodoListWidget::selectRow);
  connect(row, &TaskRowWidget::checkedChanged, this,
          &TodoListWidget::contentChanged);
  connect(row, &TaskRowWidget::taskCompleted, this,
          [this, row](const QString &text) {
            removeRow(row);
            emit taskCompleted(text);
            emit contentChanged();
          });
  int gapIdx = layout->indexOf(dropGap);
  layout->insertWidget(gapIdx, row);
}

void TodoListWidget::removeRow(TaskRowWidget *row) {
  if (selectedRow == row) {
    selectedRow = nullptr;
    emit activeTaskChanged(QString());
  }
  layout->removeWidget(row);
  row->deleteLater();
}

void TodoListWidget::clearTasks() {
  if (selectedRow) {
    selectedRow = nullptr;
    emit activeTaskChanged(QString());
  }
  LayoutUtils::clearRows(layout);
}

QList<QPair<QString, bool>> TodoListWidget::tasks() const {
  QList<QPair<QString, bool>> result;
  LayoutUtils::forEachRow<TaskRowWidget>(
      layout, [&result](TaskRowWidget *row) {
        result.append({row->text(), row->isChecked()});
      });
  return result;
}

void TodoListWidget::deselectAll() {
  if (!selectedRow) return;
  selectedRow->removeHighlight(palette().color(QPalette::Base));
  selectedRow = nullptr;
}

void TodoListWidget::adjustRowHeights() {
  LayoutUtils::adjustRowHeights(layout);
}

void TodoListWidget::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key_Delete && selectedRow) {
    layout->removeWidget(selectedRow);
    selectedRow->deleteLater();
    selectedRow = nullptr;
    emit activeTaskChanged(QString());
    emit contentChanged();
    e->accept();
    return;
  }
  QScrollArea::keyPressEvent(e);
}

void TodoListWidget::startDrag(TaskRowWidget *row) {
  if (draggedRow) return;
  draggedRow = row;
  dragOriginalIndex = layout->indexOf(row);

  QSize sz = row->size();
  if (sz.width() < viewport()->width()) {
    sz.setWidth(viewport()->width());
  }
  QPixmap pixmap(sz);
  pixmap.fill(Qt::transparent);
  row->render(&pixmap, QPoint(0, 0), QRegion(row->rect()));
  QPainter p(&pixmap);
  p.setPen(QPen(Qt::black, 1));
  p.drawRect(0, 0, sz.width() - 1, sz.height() - 1);
  p.end();

  dragGhost = new QLabel;
  dragGhost->setPixmap(pixmap);
  dragGhost->setWindowFlags(Qt::FramelessWindowHint |
                             Qt::WindowStaysOnTopHint | Qt::Tool);
  dragGhost->setAttribute(Qt::WA_TranslucentBackground);
  dragGhost->setFixedSize(pixmap.size());

  dragHandleOffset = row->dragHandleWidget()->geometry().center();

  QPoint globalPos = QCursor::pos();
  dragGhost->move(globalPos - dragHandleOffset);
  dragGhost->show();

  layout->removeWidget(row);
  row->hide();
  moveGapToRow(dragOriginalIndex);
  dropGap->setFixedHeight(Sizes::GAP_HEIGHT);

  viewport()->setCursor(Qt::ClosedHandCursor);

  dragTimer = new QTimer(this);
  dragTimer->setInterval(Sizes::DRAG_TIMER_INTERVAL);
  connect(dragTimer, &QTimer::timeout, this, &TodoListWidget::dragTick);
  dragTimer->start();
}

void TodoListWidget::dragTick() {
  if (!draggedRow) return;

  if (!(QApplication::mouseButtons() & Qt::LeftButton)) {
    finishDrag();
    return;
  }

  QPoint globalPos = QCursor::pos();
  dragGhost->move(globalPos - dragHandleOffset);

  QPoint viewportPos = viewport()->mapFromGlobal(globalPos);
  updateDropIndicator(viewportPos);
  checkAutoScroll(viewportPos.y());
}

void TodoListWidget::finishDrag() {
  dragTimer->stop();
  dragTimer->deleteLater();
  dragTimer = nullptr;

  scrollTimer->stop();
  scrollDirection = 0;

  QPoint globalPos = QCursor::pos();
  QPoint viewportPos = viewport()->mapFromGlobal(globalPos);
  int dropRow = computeDropRow(viewportPos);

  layout->removeWidget(dropGap);
  dropGap->setFixedHeight(0);

  int insertIdx = qBound(0, dropRow, layout->count() - 1);
  layout->insertWidget(insertIdx, draggedRow);
  draggedRow->show();
  layout->insertWidget(layout->count() - 1, dropGap);

  dragGhost->deleteLater();
  dragGhost = nullptr;
  viewport()->unsetCursor();
  draggedRow = nullptr;
  emit contentChanged();
}

int TodoListWidget::gapRowCount() const {
  int gapIdx = layout->indexOf(dropGap);
  int count = 0;
  for (int i = 0; i < gapIdx; ++i) {
    QLayoutItem *item = layout->itemAt(i);
    if (item && item->widget() &&
        qobject_cast<TaskRowWidget *>(item->widget())) {
      count++;
    }
  }
  return count;
}

void TodoListWidget::moveGapToRow(int rowCount) {
  layout->removeWidget(dropGap);
  int maxIdx = layout->count() - 1;
  int insertIdx = qBound(0, rowCount, maxIdx);
  layout->insertWidget(insertIdx, dropGap);
}

void TodoListWidget::updateDropIndicator(const QPoint &viewportPos) {
  int dropRow = computeDropRow(viewportPos);
  if (dropRow == gapRowCount()) return;
  moveGapToRow(dropRow);
  if (dropGap->height() != Sizes::GAP_HEIGHT) {
    dropGap->setFixedHeight(Sizes::GAP_HEIGHT);
  }
}

int TodoListWidget::computeDropRow(const QPoint &viewportPos) const {
  int containerY = widget()->y();
  int rowCount = 0;

  for (int i = 0; i < layout->count(); ++i) {
    QLayoutItem *item = layout->itemAt(i);
    if (!item || !item->widget()) continue;
    auto *row = qobject_cast<TaskRowWidget *>(item->widget());
    if (!row) continue;

    QRect geo = row->geometry();
    int mid = containerY + geo.top() + geo.height() / 2;

    if (viewportPos.y() < mid) {
      break;
    }
    rowCount++;
  }
  return rowCount;
}

void TodoListWidget::selectRow(TaskRowWidget *row) {
  if (selectedRow == row) return;

  if (selectedRow) {
    selectedRow->removeHighlight(palette().color(QPalette::Base));
  }

  selectedRow = row;

  if (selectedRow) {
    selectedRow->applyHighlight(palette().color(QPalette::Highlight),
                                 palette().color(QPalette::HighlightedText));
  }
  emit activeTaskChanged(selectedRow ? selectedRow->text() : QString());
}

void TodoListWidget::checkAutoScroll(int viewportY) {
  int h = viewport()->height();
  if (viewportY < Sizes::SCROLL_DISTANCE) {
    scrollDirection = -1;
    scrollTimer->start();
  } else if (viewportY > h - Sizes::SCROLL_DISTANCE) {
    scrollDirection = 1;
    scrollTimer->start();
  } else {
    scrollDirection = 0;
    scrollTimer->stop();
  }
}

TodoWidget::TodoWidget(const QString &todoPath, const QString &donePath,
                       QWidget *parent)
    : QWidget(parent), todoFilePath(todoPath), doneFilePath(donePath) {
  listWidget = new TodoListWidget(this);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);
  mainLayout->addWidget(listWidget);

  auto *inputContainer = new QWidget(this);
  inputContainer->setObjectName("taskInputContainer");
  inputContainer->setStyleSheet(Styles::INPUT_CONTAINER);
  inputContainer->setFixedHeight(32);

  auto *inputLayout = new QHBoxLayout(inputContainer);
  inputLayout->setContentsMargins(8, 0, 4, 0);
  inputLayout->setSpacing(0);

  taskInput = new QLineEdit(inputContainer);
  taskInput->setPlaceholderText(tr("Add new task..."));
  taskInput->setFrame(false);
  taskInput->setStyleSheet("QLineEdit { border: none; background: transparent; }");

  addButton = new QToolButton(inputContainer);
  addButton->setText("+");
  addButton->setFixedSize(24, 24);
  addButton->setStyleSheet(Styles::ADD_BUTTON);

  inputLayout->addWidget(taskInput, 1);
  inputLayout->addWidget(addButton);

  auto *bottomWidget = new QWidget(this);
  bottomWidget->setAutoFillBackground(true);
  QPalette bwPalette = bottomWidget->palette();
  bwPalette.setColor(QPalette::Window, Qt::white);
  bottomWidget->setPalette(bwPalette);

  auto *bottomLayout = new QHBoxLayout(bottomWidget);
  bottomLayout->setContentsMargins(8, 4, 8, 4);
  bottomLayout->setSpacing(0);
  bottomLayout->addWidget(inputContainer);

  mainLayout->addWidget(bottomWidget);

  connect(taskInput, &QLineEdit::returnPressed, this, &TodoWidget::addNewTask);
  connect(addButton, &QToolButton::clicked, this, &TodoWidget::addNewTask);
  connect(listWidget, &TodoListWidget::contentChanged, this, &TodoWidget::saveToFile);
  connect(listWidget, &TodoListWidget::taskCompleted, this, &TodoWidget::onTaskCompleted);
  connect(listWidget, &TodoListWidget::activeTaskChanged, this,
          &TodoWidget::activeTaskChanged);

  fileMonitor = new FileMonitor(todoFilePath, this);
  connect(fileMonitor, &FileMonitor::changedExternally, this, &TodoWidget::reload);
  fileMonitor->setActive(
      Paths::settings()->value(SettingsKeys::FILE_WATCHER,
                                SettingsKeys::FILE_WATCHER_DEFAULT)
          .toBool());

  auto *inactiveTimer = new QTimer(this);
  inactiveTimer->setInterval(200);
  connect(inactiveTimer, &QTimer::timeout, this, [this]() {
    if (!window()->isActiveWindow()) {
      listWidget->deselectAll();
    }
  });
  inactiveTimer->start();

  loadFromFile();
}

void TodoWidget::loadFromFile() {
  const auto content = FileUtils::readFile(todoFilePath);
  if (!content) return;

  const QString donePrefix = QLatin1String(Formats::DONE_PREFIX);
  for (const QString &line : content->split('\n')) {
    const QString l = line.trimmed();
    if (l.isEmpty()) continue;
    if (l.startsWith(donePrefix)) {
      listWidget->addTask(l.mid(donePrefix.size()), true);
    } else {
      listWidget->addTask(l, false);
    }
  }
  fileMonitor->setLastKnownContent(*content);
  QTimer::singleShot(0, this, [this]() { listWidget->adjustRowHeights(); });
}

void TodoWidget::saveToFile() {
  QString content;
  const QString donePrefix = QLatin1String(Formats::DONE_PREFIX);
  for (const auto &task : listWidget->tasks()) {
    if (task.second) {
      content += donePrefix + task.first + "\n";
    } else {
      content += task.first + "\n";
    }
  }

  if (FileUtils::writeFile(todoFilePath, content)) {
    fileMonitor->setLastKnownContent(content);
  }
}

void TodoWidget::addNewTask() {
  QString text = taskInput->text().trimmed();
  if (text.isEmpty()) return;
  listWidget->addTask(text, false);
  taskInput->clear();
  saveToFile();
}

void TodoWidget::onTaskCompleted(const QString &text) {
  if (FileUtils::appendFile(
          doneFilePath, DateUtils::currentDateStr() + " " + text + "\n")) {
    emit doneListChanged();
  }
}

void TodoWidget::reload() {
  listWidget->clearTasks();
  loadFromFile();
}

void TodoWidget::setWatcherEnabled(bool enabled) {
  fileMonitor->setActive(enabled);
}

DoneRowWidget::DoneRowWidget(const QString &originalLine, QWidget *parent)
    : BaseRowWidget(parent), fullLine(originalLine) {
  taskText = DateUtils::stripDate(originalLine);

  checkbox->setStyleSheet(Styles::CHECKBOX_DONE);
  checkbox->setChecked(true);
  connect(checkbox, &QCheckBox::toggled, this, [this](bool checked) {
    if (!checked) emit taskUncompleted(taskText);
  });

  label->setText(taskText);
  label->setStyleSheet("text-decoration: line-through;");
}

DoneWidget::DoneWidget(const QString &donePath, const QString &todoPath,
                       QWidget *parent)
    : QWidget(parent), doneFilePath(donePath), todoFilePath(todoPath) {
  scrollArea = new QScrollArea(this);
  container = UiUtils::setupListScrollArea(scrollArea);

  layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addStretch(1);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(scrollArea);

  loadFromFile();

  cleanupDays = Paths::settings()
                    ->value(SettingsKeys::CLEANUP_DAYS,
                             SettingsKeys::CLEANUP_DAYS_DEFAULT)
                    .toInt();
  if (Paths::settings()
          ->value(SettingsKeys::AUTO_CLEANUP, SettingsKeys::AUTO_CLEANUP_DEFAULT)
          .toBool()) {
    cleanupOldTasks();
  }
}

void DoneWidget::loadFromFile() {
  const auto content = FileUtils::readFile(doneFilePath);
  if (!content) return;

  for (const QString &raw : content->split('\n')) {
    const QString line = raw.trimmed();
    if (line.isEmpty()) continue;
    auto *row = new DoneRowWidget(line, container);
    connect(row, &DoneRowWidget::taskUncompleted, this,
            &DoneWidget::onTaskUncompleted);
    layout->insertWidget(layout->count() - 1, row);
  }
  QTimer::singleShot(0, this, [this]() { adjustRowHeights(); });
}

void DoneWidget::adjustRowHeights() {
  LayoutUtils::adjustRowHeights(layout);
}

void DoneWidget::saveToFile() {
  QString content;
  LayoutUtils::forEachRow<DoneRowWidget>(
      layout, [&content](DoneRowWidget *row) {
        content += row->originalLine() + "\n";
      });

  FileUtils::writeFile(doneFilePath, content);
}

void DoneWidget::clearTasks() {
  LayoutUtils::clearRows(layout);
}

void DoneWidget::reload() {
  clearTasks();
  loadFromFile();
}

void DoneWidget::onTaskUncompleted(const QString &text) {
  FileUtils::appendFile(todoFilePath, text + "\n");

  auto *row = qobject_cast<DoneRowWidget *>(sender());
  if (row) {
    layout->removeWidget(row);
    row->deleteLater();
  }
  saveToFile();
  emit todoListChanged();
}

void DoneWidget::setAutoCleanup(bool enabled, int days) {
  cleanupDays = days;
  if (enabled) cleanupOldTasks();
}

void DoneWidget::cleanupOldTasks() {
  const auto content = FileUtils::readFile(doneFilePath);
  if (!content) return;

  QStringList kept;
  bool changed = false;
  const QDate cutoff = QDate::currentDate().addDays(-cleanupDays);
  for (const QString &raw : content->split('\n')) {
    const QString line = raw.trimmed();
    if (line.isEmpty()) continue;
    const QDate date = DateUtils::dateFromLine(line);
    if (date.isValid() && date < cutoff) {
      changed = true;
      continue;
    }
    kept.append(line);
  }

  if (!changed) return;

  QString newContent = kept.join("\n");
  if (!kept.isEmpty()) newContent += "\n";

  if (FileUtils::writeFile(doneFilePath, newContent)) {
    clearTasks();
    loadFromFile();
  }
}
