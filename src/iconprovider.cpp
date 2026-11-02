#include "iconprovider.h"
#include "utils.h"

#include <QFile>
#include <QIcon>

QIcon IconProvider::icon(const QString &fileName) {
  const QString path = Paths::iconsDir() + "/" + fileName;
  if (QFile::exists(path)) {
    return QIcon(path);
  }
  return QIcon(QStringLiteral(":/icons/") + fileName);
}
