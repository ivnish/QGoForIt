#include "baserowwidget.h"
#include "constants.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QTimer>

BaseRowWidget::BaseRowWidget(QWidget *parent) : QWidget(parent) {
  setFixedHeight(Sizes::ROW_HEIGHT);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  hLayout = new QHBoxLayout(this);
  hLayout->setContentsMargins(8, 4, 4, 4);
  hLayout->setSpacing(8);

  checkbox = new QCheckBox(this);
  checkbox->setFixedSize(Sizes::CHECKBOX_SIZE, Sizes::CHECKBOX_SIZE);

  label = new QLabel(this);
  label->setWordWrap(true);
  QFont labelFont = label->font();
  labelFont.setPointSize(labelFont.pointSize() + 1);
  label->setFont(labelFont);

  hLayout->addWidget(checkbox);
  hLayout->addWidget(label, 1);
}

void BaseRowWidget::resizeEvent(QResizeEvent *e) {
  QWidget::resizeEvent(e);
  QTimer::singleShot(0, this, [this]() { adjustHeight(); });
}

void BaseRowWidget::adjustHeight() {
  if (adjustingHeight) return;
  adjustingHeight = true;
  int labelHeight = label->heightForWidth(label->width());
  if (labelHeight < 0) labelHeight = label->sizeHint().height();
  int needed = qMax(Sizes::ROW_HEIGHT, labelHeight + Sizes::ROW_MARGINS_TOTAL);
  if (height() != needed) {
    setFixedHeight(needed);
  }
  adjustingHeight = false;
}
