#ifndef METACONTACTS_H
#define METACONTACTS_H

#include <QMap>
#include <QHash>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/istatusicons.h>
#include "combinecontactsdialog.h"

class MetaContacts : 
	public QObject,
	public IPlugin,
	public IMetaContacts,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMetaContacts IRosterDataHolder);
public:
	MetaContacts();
	~MetaContacts();
	virtual QObject *instance() { return this; }
	// IPlugin
	virtual QUuid pluginUuid() const { return METACONTACTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	// IMetaContacts
	virtual bool isReady(const Jid &AStreamJid) const;
	virtual IMetaContact findMetaContact(const Jid &AStreamJid, const Jid &AItem) const;
	virtual IMetaContact findMetaContact(const Jid &AStreamJid, const QUuid &AMetaId) const;
	virtual QList<IRosterIndex *> findMetaIndexes(const Jid &AStreamJid, const QUuid &AMetaId) const;
	virtual QUuid createMetaContact(const Jid &AStreamJid, const QList<Jid> &AItems, const QString &AName);
	virtual bool mergeMetaContacts(const Jid &AStreamJid, const QUuid &AMetaId1, const QUuid &AMetaId2);
	virtual bool detachMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems);
	virtual bool setMetaContactName(const Jid &AStreamJid, const QUuid &AMetaId, const QString &AName);
	virtual bool setMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems);
	virtual bool setMetaContactGroups(const Jid &AStreamJid, const QUuid &AMetaId, const QSet<QString> &AGroups);
signals:
	void metaContactChanged(const Jid &AStreamJid, const IMetaContact &AMetaContact, const IMetaContact &ABefore);
	// IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
protected:
	bool isValidItem(const Jid &AStreamJid, const Jid &AItem) const;
	bool updateMetaContact(const Jid &AStreamJid, const IMetaContact &AMetaContact);
	void updateMetaContacts(const Jid &AStreamJid, const QList<IMetaContact> &AMetaContacts);
protected:
	bool isReadyStreams(const QStringList &AStreams) const;
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	void updateMetaIndex(const Jid &AStreamJid, const IMetaContact &AMetaContact);
	void removeMetaIndex(const Jid &AStreamJid, const QUuid &AMetaId);
	void showCombineContactsDialog(const Jid &AStreamJid, const QStringList &AContactJids, const QStringList &AMetaIds);
protected:
	void startSaveContactsToStorage(const Jid &AStreamJid);
	bool saveContactsToStorage(const Jid &AStreamJid) const;
	QString metaContactsFileName(const Jid &AStreamJid) const;
	QList<IMetaContact> loadMetaContactsFromXML(const QDomElement &AElement) const;
	void saveMetaContactsToXML(QDomElement &AElement, const QList<IMetaContact> &AContacts) const;
	QList<IMetaContact> loadMetaContactsFromFile(const QString &AFileName) const;
	void saveMetaContactsToFile(const QString &AFileName, const QList<IMetaContact> &AContacts) const;
protected slots:
	void onRosterAdded(IRoster *ARoster);
	void onRosterRemoved(IRoster *ARoster);
	void onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
protected slots:
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
protected slots:
	void onPrivateStorageOpened(const Jid &AStreamJid);
	void onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onPrivateStorageNotifyAboutToClose(const Jid &AStreamJid);
protected slots:
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
protected slots:
	void onCombineContactsByAction();
protected slots:
	void onLoadContactsFromFileTimerTimeout();
	void onSaveContactsToStorageTimerTimeout();
private:
	IPluginManager *FPluginManager;
	IPrivateStorage *FPrivateStorage;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IStatusIcons *FStatusIcons;
private:
	QTimer FSaveTimer;
	QSet<Jid> FSaveStreams;
	QSet<Jid> FLoadStreams;
private:
	QMap<Jid, QHash<Jid, QUuid> > FItemMetaContact;
	QMap<Jid, QHash<QUuid, IMetaContact> > FMetaContacts;
	QMap<Jid, QHash<QUuid, QList<IRosterIndex *> > > FMetaIndexes;
};

#endif // METACONTACTS_H
