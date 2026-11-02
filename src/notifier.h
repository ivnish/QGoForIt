#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <QObject>

/**
 * Sends desktop notifications through the org.freedesktop.Notifications
 * D-Bus service (KDE Plasma notification manager, GNOME, etc.).
 *
 * Calls are fully asynchronous, so a hanging notification service can
 * never freeze the UI thread.
 *
 * The same replacement id is reused, so each new notification replaces
 * the previous one, similar to GoForIt's single notification.
 *
 * When D-Bus support is unavailable at build time or the service is not
 * reachable at runtime, falls back to a system beep.
 */
class Notifier : public QObject {
  Q_OBJECT

 public:
  explicit Notifier(QObject *parent = nullptr);

 public slots:
  void notify(const QString &summary, const QString &body);

 private:
  quint32 m_replacesId = 0;
};

#endif  // NOTIFIER_H
