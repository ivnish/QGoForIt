#ifndef BASEROWWIDGET_H
#define BASEROWWIDGET_H

#include <QWidget>

class QCheckBox;
class QLabel;
class QHBoxLayout;
class QResizeEvent;

class BaseRowWidget : public QWidget {
  Q_OBJECT

 public:
  explicit BaseRowWidget(QWidget *parent = nullptr);
  void adjustHeight();

 protected:
  void resizeEvent(QResizeEvent *e) override;

  QCheckBox *checkbox;
  QLabel *label;
  QHBoxLayout *hLayout;
  bool adjustingHeight = false;
};

#endif  // BASEROWWIDGET_H
