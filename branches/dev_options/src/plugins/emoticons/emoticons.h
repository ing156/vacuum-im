#ifndef EMOTICONS_H
#define EMOTICONS_H

#include <QMap>
#include <QHash>
#include <QStringList>
#include <definations/actiongroups.h>
#include <definations/toolbargroups.h>
#include <definations/messagewriterorders.h>
#include <definations/resourceloaderorders.h>
#include <definations/optionvalues.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iemoticons.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/iconstorage.h>
#include <utils/options.h>
#include <utils/menu.h>
#include "selecticonmenu.h"
#include "emoticonsoptions.h"

class Emoticons : 
  public QObject,
  public IPlugin,
  public IEmoticons,
  public IMessageWriter,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IEmoticons IMessageWriter IOptionsHolder);
public:
  Emoticons();
  ~Emoticons();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return EMOTICONS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings();
  virtual bool startPlugin() { return true; }
  //IMessageWriter
  virtual void writeMessage(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder);
  virtual void writeText(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder);
  //IOptionsHolder
  virtual IOptionsWidget *optionsWidget(const QString &ANodeId, int &AOrder, QWidget *AParent);
  //IEmoticons
  virtual QList<QString> activeIconsets() const;
  virtual QUrl urlByKey(const QString &AKey) const;
  virtual QString keyByUrl(const QUrl &AUrl) const;
protected:
  void createIconsetUrls();
  SelectIconMenu *createSelectIconMenu(const QString &ASubStorage, QWidget *AParent);
  void insertSelectIconMenu(const QString &ASubStorage);
  void removeSelectIconMenu(const QString &ASubStorage);
protected slots:
  void onToolBarWidgetCreated(IToolBarWidget *AWidget);
  void onToolBarWidgetDestroyed(QObject *AObject);
  void onIconSelected(const QString &ASubStorage, const QString &AIconKey);
  void onSelectIconMenuDestroyed(QObject *AObject);
  void onOptionsOpened();
  void onOptionsChanged(const OptionsNode &ANode);
private:
  IMessageWidgets *FMessageWidgets;
  IMessageProcessor *FMessageProcessor;
  IOptionsManager *FOptionsManager;
private:
  QHash<QString, QUrl> FUrlByKey;
  QMap<QString, IconStorage *> FStorages;
  QList<IToolBarWidget *> FToolBarsWidgets;
  QMap<SelectIconMenu *, IToolBarWidget *> FToolBarWidgetByMenu;
};

#endif // EMOTICONS_H