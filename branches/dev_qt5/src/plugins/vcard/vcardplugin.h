#ifndef VCARDPLUGIN_H
#define VCARDPLUGIN_H

#include <QTimer>
#include <QObjectCleanupHandler>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/xmppurihandlerorders.h>
#include <definitions/toolbargroups.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ivcard.h>
#include <interfaces/iroster.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/irostersview.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/imessagewidgets.h>
#include <utils/widgetmanager.h>
#include <utils/stanza.h>
#include <utils/action.h>
#include <utils/shortcuts.h>
#include <utils/xmpperror.h>
#include "vcard.h"
#include "vcarddialog.h"

struct VCardItem {
	VCardItem() {
		vcard = NULL;
		locks = 0;
	}
	VCard *vcard;
	int locks;
};

class VCardPlugin :
	public QObject,
	public IPlugin,
	public IVCardPlugin,
	public IStanzaRequestOwner,
	public IXmppUriHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IVCardPlugin IStanzaRequestOwner IXmppUriHandler);
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IVCardPlugin");
#endif
	friend class VCard;
public:
	VCardPlugin();
	~VCardPlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return VCARD_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin()  { return true; }
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IVCardPlugin
	virtual QString vcardFileName(const Jid &AContactJid) const;
	virtual bool hasVCard(const Jid &AContactJid) const;
	virtual IVCard *getVCard(const Jid &AContactJid);
	virtual bool requestVCard(const Jid &AStreamJid, const Jid &AContactJid);
	virtual bool publishVCard(IVCard *AVCard, const Jid &AStreamJid);
	virtual void showVCardDialog(const Jid &AStreamJid, const Jid &AContactJid);
signals:
	void vcardReceived(const Jid &AContactJid);
	void vcardPublished(const Jid &AContactJid);
	void vcardError(const Jid &AContactJid, const XmppError &AError);
protected:
	void registerDiscoFeatures();
	void unlockVCard(const Jid &AContactJid);
	void saveVCardFile(const Jid &AContactJid, const QDomElement &AElem) const;
	void removeEmptyChildElements(QDomElement &AElem) const;
	void insertMessageToolBarAction(IMessageToolBarWidget *AWidget);
protected slots:
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
	void onShowVCardDialogByAction(bool);
	void onShowVCardDialogByMessageWindowAction(bool);
	void onVCardDialogDestroyed(QObject *ADialog);
	void onXmppStreamRemoved(IXmppStream *AXmppStream);
	void onMessageNormalWindowCreated(IMessageNormalWindow *AWindow);
	void onMessageChatWindowCreated(IMessageChatWindow *AWindow);
protected slots:
	void onUpdateTimerTimeout();
	void onRosterOpened(IRoster *ARoster);
	void onRosterClosed(IRoster *ARoster);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	IRosterPlugin *FRosterPlugin;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IStanzaProcessor *FStanzaProcessor;
	IMultiUserChatPlugin *FMultiUserChatPlugin;
	IServiceDiscovery *FDiscovery;
	IXmppUriQueries *FXmppUriQueries;
	IMessageWidgets *FMessageWidgets;
private:
	QTimer FUpdateTimer;
	QMap<Jid,VCardItem> FVCards;
	QMultiMap<Jid,Jid> FUpdateQueue;
	QMap<QString,Jid> FVCardRequestId;
	QMap<QString,Jid> FVCardPublishId;
	QMap<QString,Stanza> FVCardPublishStanza;
	QMap<Jid,VCardDialog *> FVCardDialogs;
};

#endif // VCARDPLUGIN_H