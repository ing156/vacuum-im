#ifndef INOTIFICATIONS_H
#define INOTIFICATIONS_H

#include <QMap>
#include <QIcon>
#include <QImage>
#include <utils/jid.h>

#define NOTIFICATIONS_UUID  "{59887A91-A483-4a7c-A2DE-227A01D6BC5E}"

struct INotification 
{
	enum NotifyKinds {
		RosterIcon    = 0x01,
		PopupWindow   = 0x02,
		TrayIcon      = 0x04,
		TrayAction    = 0x08,
		PlaySound     = 0x10,
		AutoActivate  = 0x20
	};
	INotification() {
		kinds = 0; 
	}
	uchar kinds;
	QString type;
	QMap<int, QVariant> data;
};

class INotificationHandler
{
public:
	virtual bool showNotification(int AOrder, uchar AKind, int ANotifyId, const INotification &ANotification) =0;
};

class INotifications 
{
public:
	virtual QObject *instance() =0;
	virtual QList<int> notifications() const =0;
	virtual INotification notificationById(int ANotifyId) const =0;
	virtual int appendNotification(const INotification &ANotification) =0;
	virtual void activateNotification(int ANotifyId) =0;
	virtual void removeNotification(int ANotifyId) =0;
	virtual void registerNotificationType(const QString &AType, int AOptionsOrder, const QString &ATitle, uchar AKindMask, uchar ADefault) =0;
	virtual uchar notificationKinds(const QString &AType) const =0;
	virtual void setNotificationKinds(const QString &AType, uchar AKinds) =0;
	virtual void removeNotificationType(const QString &AType) =0;
	virtual void insertNotificationHandler(int AOrder, INotificationHandler *AHandler) =0;
	virtual void removeNotificationHandler(int AOrder, INotificationHandler *AHandler) =0;
	virtual QImage contactAvatar(const Jid &AContactJid) const =0;
	virtual QIcon contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QString contactName(const Jid &AStreamJId, const Jid &AContactJid) const =0;
protected:
	virtual void notificationActivated(int ANotifyId) =0;
	virtual void notificationRemoved(int ANotifyId) =0;
	virtual void notificationAppend(int ANotifyId, INotification &ANotification) =0;
	virtual void notificationAppended(int ANotifyId, const INotification &ANotification) =0;
	virtual void notificationHandlerInserted(int AOrder, INotificationHandler *AHandler) =0;
	virtual void notificationHandlerRemoved(int AOrder, INotificationHandler *AHandler) =0;
};

Q_DECLARE_INTERFACE(INotificationHandler,"Vacuum.Plugin.INotificationHandler/1.0")
Q_DECLARE_INTERFACE(INotifications,"Vacuum.Plugin.INotifications/1.0")

#endif //INOTIFICATIONS_H