#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H

#include <QIcon>
#include <QString>

class IconProvider {
 public:
  static QIcon icon(const QString &fileName);
};

#endif // ICONPROVIDER_H
