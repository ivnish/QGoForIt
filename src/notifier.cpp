#include "notifier.h"
#include "constants.h"

#include <QApplication>

#ifdef QGOFORIT_HAVE_DBUS
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#endif

Notifier::Notifier(QObject *parent) : QObject(parent) {}

void Notifier::notify(const QString &summary, const QString &body) {
#ifdef QGOFORIT_HAVE_DBUS
  if (QDBusConnection::sessionBus().isConnected()) {
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("Notify"));

    QVariantMap hints;
    hints.insert(QStringLiteral("urgency"), 1);  // normal
    msg.setArguments({QStringLiteral("QGoForIt"),  // app name
                      m_replacesId,                // replaces id
                      QStringLiteral("QGoForIt"),  // icon
                      summary, body, QStringList(), hints,
                      Sizes::NOTIFICATION_TIMEOUT_MS});

    auto *watcher = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *call) {
              QDBusPendingReply<quint32> reply = *call;
              if (reply.isValid()) {
                m_replacesId = reply.value();
              } else {
                QApplication::beep();
              }
              call->deleteLater();
            });
    return;
  }
#endif
  QApplication::beep();
}
