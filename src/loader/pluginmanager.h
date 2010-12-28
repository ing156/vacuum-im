#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QHash>
#include <QPointer>
#include <QTranslator>
#include <QDomDocument>
#include <QApplication>
#include <QPluginLoader>
#include <QSessionManager>
#include <definitions/plugininitorders.h>
#include <definitions/commandline.h>
#include <definitions/actiongroups.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/version.h>
#include <definitions/shortcuts.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/itraymanager.h>
#include <utils/widgetmanager.h>
#include <utils/filestorage.h>
#include <utils/shortcuts.h>
#include <utils/action.h>
#include "setuppluginsdialog.h"
#include "aboutbox.h"

struct PluginItem
{
	IPlugin *plugin;
	IPluginInfo *info;
	QPluginLoader *loader;
	QTranslator *translator;
};

class PluginManager :
			public QObject,
			public IPluginManager
{
	Q_OBJECT;
	Q_INTERFACES(IPluginManager);
public:
	PluginManager(QApplication *AParent);
	~PluginManager();
	virtual QObject *instance() { return this; }
	virtual QString version() const;
	virtual QString revision() const;
	virtual QDateTime revisionDate() const;
	virtual QString homePath() const;
	virtual void setHomePath(const QString &APath);
	virtual void setLocale(QLocale::Language ALanguage, QLocale::Country ACountry);
	virtual IPlugin* pluginInstance(const QUuid &AUuid) const;
	virtual QList<IPlugin *> pluginInterface(const QString &AInterface = QString::null) const;
	virtual const IPluginInfo *pluginInfo(const QUuid &AUuid) const;
	virtual QList<QUuid> pluginDependencesOn(const QUuid &AUuid) const;
	virtual QList<QUuid> pluginDependencesFor(const QUuid &AUuid) const;
public slots:
	virtual void quit();
	virtual void restart();
signals:
	void aboutToQuit();
protected:
	void loadSettings();
	void saveSettings();
	void loadPlugins();
	bool initPlugins();
	void startPlugins();
protected:
	void removePluginItem(const QUuid &AUuid, const QString &AError);
	void unloadPlugin(const QUuid &AUuid, const QString &AError = QString::null);
	bool checkDependences(const QUuid AUuid) const;
	bool checkConflicts(const QUuid AUuid) const;
	QList<QUuid> getConflicts(const QUuid AUuid) const;
	void loadCoreTranslations(const QString &ADir);
protected:
	bool isPluginEnabled(const QString &AFile) const;
	QDomElement savePluginInfo(const QString &AFile, const IPluginInfo *AInfo);
	void savePluginError(const QString &AFile, const QString &AEror);
	void removePluginsInfo(const QStringList &ACurFiles);
	void createMenuActions();
	void declareShortcuts();
protected slots:
	void onApplicationAboutToQuit();
	void onApplicationCommitDataRequested(QSessionManager &AManager);
	void onShowSetupPluginsDialog(bool);
	void onSetupPluginsDialogAccepted();
	void onShowAboutBoxDialog();
private:
	QPointer<AboutBox> FAboutDialog;
	QPointer<SetupPluginsDialog> FPluginsDialog;
private:
	QString FDataPath;
	QDomDocument FPluginsSetup;
	QTranslator *FQtTranslator;
	QTranslator *FUtilsTranslator;
	QTranslator *FLoaderTranslator;
	QList<QString> FBlockedPlugins;
	QHash<QUuid, PluginItem> FPluginItems;
	mutable QMultiHash<QString, IPlugin *> FPlugins;
};

#endif
