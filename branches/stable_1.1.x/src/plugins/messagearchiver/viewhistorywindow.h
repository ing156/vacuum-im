#ifndef VIEWHISTORYWINDOW_H
#define VIEWHISTORYWINDOW_H

#include <QTimer>
#include <QSortFilterProxyModel>
#include <definitions/namespaces.h>
#include <definitions/archiveindextyperole.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/istatusicons.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/imessagestyles.h>
#include <utils/options.h>
#include <utils/widgetmanager.h>
#include "ui_viewhistorywindow.h"

class ViewHistoryWindow;

class SortFilterProxyModel :
			public QSortFilterProxyModel
{
public:
	SortFilterProxyModel(ViewHistoryWindow *AWindow, QObject *AParent = NULL);
protected:
	virtual bool filterAcceptsRow(int ARow, const QModelIndex &AParent) const;
private:
	ViewHistoryWindow *FWindow;
};

struct ViewOptions {
	bool isGroupchat;
	QString selfName;
	QString selfAvatar;
	QString contactName;
	QString contactAvatar;
};

class ViewHistoryWindow :
			public QMainWindow,
			public IArchiveWindow
{
	Q_OBJECT;
	Q_INTERFACES(IArchiveWindow);
public:
	ViewHistoryWindow(IMessageArchiver *AArchiver, IPluginManager *APluginManager, const Jid &AStreamJid, QWidget *AParent = NULL);
	~ViewHistoryWindow();
	virtual QMainWindow *instance() { return this; }
	virtual const Jid &streamJid() const;
	virtual ToolBarChanger *collectionTools() const;
	virtual ToolBarChanger *messagesTools() const;
	virtual bool isHeaderAccepted(const IArchiveHeader &AHeader) const;
	virtual QList<IArchiveHeader> currentHeaders() const;
	virtual QStandardItem *findHeaderItem(const IArchiveHeader &AHeader, QStandardItem *AParent = NULL) const;
	virtual int groupKind() const;
	virtual void setGroupKind(int AGroupKind);
	virtual int archiveSource() const;
	virtual void setArchiveSource(int ASource);
	virtual const IArchiveFilter &filter() const;
	virtual void setFilter(const IArchiveFilter &AFilter);
	virtual void reload();
signals:
	void groupKindChanged(int AGroupKind);
	void archiveSourceChanged(int ASource);
	void filterChanged(const IArchiveFilter &AFilter);
	void itemCreated(QStandardItem *AItem);
	void itemContextMenu(QStandardItem *AItem, Menu *AMenu);
	void currentItemChanged(QStandardItem *ACurrent, QStandardItem *ABefore);
	void itemDestroyed(QStandardItem *AItem);
	void windowDestroyed(IArchiveWindow *AWindow);
protected:
	void initialize(IPluginManager *APluginManager);
	QList<IArchiveHeader> indexHeaders(const QModelIndex &AIndex) const;
	QList<IArchiveRequest> createRequests(const IArchiveFilter &AFilter) const;
	void divideRequests(const QList<IArchiveRequest> &ARequests, QList<IArchiveRequest> &ALocal, QList<IArchiveRequest> &AServer) const;
	bool loadServerHeaders(const IArchiveRequest &ARequest, const QString &AAfter = "");
	bool loadServerCollection(const IArchiveHeader &AHeader, const QString &AAfter = "");
	QString contactName(const Jid &AContactJid, bool ABare = false) const;
	QStandardItem *findChildItem(int ARole, const QVariant &AValue, QStandardItem *AParent) const;
	QStandardItem *createSortItem(const QVariant &ASortData);
	QStandardItem *createCustomItem(int AType, const QVariant &ADiaplay);
	QStandardItem *createDateGroup(const IArchiveHeader &AHeader, QStandardItem *AParent);
	QStandardItem *createContactGroup(const IArchiveHeader &AHeader, QStandardItem *AParent);
	QStandardItem *createHeaderParent(const IArchiveHeader &AHeader, QStandardItem *AParent);
	QStandardItem *createHeaderItem(const IArchiveHeader &AHeader);
	void updateHeaderItem(const IArchiveHeader &AHeader);
	void removeCustomItem(QStandardItem *AItem);
	void setViewOptions(const IArchiveCollection &ACollection);
	void setMessageStyle();
	void showNotification(const QString &AMessage);
	void processRequests(const QList<IArchiveRequest> &ARequests);
	void processHeaders(const QList<IArchiveHeader> &AHeaders);
	void processCollection(const IArchiveCollection &ACollection, bool AAppend = false);
	void insertContact(const Jid &AContactJid);
	void updateFilterWidgets();
	void updateHeaderInfoWidget(const IArchiveHeader &AHeader);
	void clearModel();
	void rebuildModel();
	void createGroupKindMenu();
	void createSourceMenu();
	void createHeaderActions();
protected slots:
	void onLocalCollectionSaved(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	void onLocalCollectionRemoved(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	void onServerHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const IArchiveResultSet &AResult);
	void onServerCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const IArchiveResultSet &AResult);
	void onServerCollectionSaved(const QString &AId, const IArchiveHeader &AHeader);
	void onServerCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
	void onRequestFailed(const QString &AId, const QString &AError);
	void onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &ABefore);
	void onItemContextMenuRequested(const QPoint &APos);
	void onApplyFilterClicked();
	void onInvalidateTimeout();
	void onChangeGroupKindByAction(bool);
	void onChangeSourceByAction(bool);
	void onHeaderActionTriggered(bool);
	void onArchivePrefsChanged(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs);
	void onStreamClosed();
private:
	Ui::ViewHistoryWindowClass ui;
private:
	IRoster *FRoster;
	IMessageWidgets *FMessageWidgets;
	IMessageStyles *FMessageStyles;
	IStatusIcons *FStatusIcons;
	IMessageArchiver *FArchiver;
private:
	IViewWidget *FViewWidget;
	IToolBarWidget *FMessagesTools;
private:
	QStandardItemModel *FModel;
	SortFilterProxyModel *FProxyModel;
private:
	Action *FFilterBy;
	Action *FRename;
	Action *FRemove;
	Action *FReload;
	Menu *FGroupKindMenu;
	Menu *FSourceMenu;
	ToolBarChanger *FCollectionTools;
private:
	QMap<QString,IArchiveHeader> FRenameRequests;
	QMap<QString,IArchiveHeader> FRemoveRequests;
	QMap<QString,IArchiveRequest> FHeaderRequests;
	QMap<QString,IArchiveHeader> FCollectionRequests;
private:
	int FSource;
	int FGroupKind;
	Jid FStreamJid;
	IArchiveFilter FFilter;
	QTimer FInvalidateTimer;
	ViewOptions FViewOptions;
	QList<IArchiveRequest> FRequestList;
	QList<IArchiveHeader> FCurrentHeaders;
	QMap<IArchiveHeader,IArchiveCollection> FCollections;
};

#endif // VIEWHISTORYWINDOW_H