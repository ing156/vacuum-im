#include "filestorage.h"

#include <QDir>
#include <QDomDocument>
#include <QApplication>

QHash<QString, FileStorage *> FileStorage::FStaticStorages;
QList<QString> FileStorage::FObjectTags = QList<QString>() << "file" << "icon";
QList<QString> FileStorage::FKeyTags    = QList<QString>() << "key"  << "text" << "name";
QList<QString> FileStorage::FFileTags   = QList<QString>() << "object";

FileStorage::FileStorage(const QString &AStorage, const QString &ASubStorage, QObject *AParent) : QObject(AParent)
{
  FStorage = AStorage;
  setSubStorage(ASubStorage);
}

FileStorage::~FileStorage()
{

}

QString FileStorage::storage() const
{
  return FStorage;
}

QString FileStorage::subStorage() const
{
  return FSubStorage;
}

void FileStorage::setSubStorage(const QString &ASubStorage)
{
  if (FSubStorage.isNull() || FSubStorage!=ASubStorage)
  {
    FObjects.clear();
    FKey2Object.clear();
    FOptions.clear();
    FSubStorage = ASubStorage;
    QDir dir(qApp->applicationDirPath());
    if (dir.cd(STORAGE_DIR) && dir.cd(FStorage))
    {
      if (FSubStorage!=STORAGE_SHARED_DIR && !FSubStorage.isEmpty() && dir.cd(FSubStorage))
      {
        QStringList defFiles = dir.entryList(QStringList() << STORAGE_DEFFILES_MASK);
        foreach(QString file, defFiles) {
          loadDefinations(dir.absoluteFilePath(file),FSubStorage+"/"); }
        dir.cdUp();
      }
      if (dir.cd(STORAGE_SHARED_DIR))
      {
        QStringList defFiles = dir.entryList(QStringList() << STORAGE_DEFFILES_MASK);
        foreach(QString file, defFiles) {
          loadDefinations(dir.absoluteFilePath(file),STORAGE_SHARED_DIR"/"); }
        dir.cdUp();
      }
      FFilePrefix = dir.absolutePath()+"/";
    }
    emit storageChanged();
  }
}

QString FileStorage::storageRootDir() const
{
  return FFilePrefix;
}

QString FileStorage::option(const QString &AOption) const
{
  return FOptions.value(AOption);
}

QList<QString> FileStorage::fileKeys() const
{
  return FKeys;
}

QList<QString> FileStorage::fileFirstKeys() const
{
  QList<QString> keys = FKeys;
  int lastIndex = -1;
  QList<QString>::iterator it = keys.begin();
  while (it<keys.end())
  {
    int index = FKey2Object.value(*it);
    if (index != lastIndex)
    {
      lastIndex = index;
      it++;
    }
    else
    {
      it = keys.erase(it);
    }
  }
  return keys;
}

int FileStorage::filesCount(const QString AKey) const
{
  return FObjects.value(FKey2Object.value(AKey)).fileNames.count();
}

QString FileStorage::fileName(const QString AKey, int AIndex) const
{
  return FObjects.value(FKey2Object.value(AKey,-1)).fileNames.value(AIndex);
}

QString FileStorage::fileFullName(const QString AKey, int AIndex) const
{
  QString name = fileName(AKey,AIndex);
  return !name.isEmpty() ? FFilePrefix + name : name;
}

QString FileStorage::fileMime(const QString AKey, int AIndex) const
{
  return FObjects.value(FKey2Object.value(AKey)).fileMimes.value(AIndex);
}

QString FileStorage::fileOption(const QString AKey, const QString &AOption) const
{
  return FObjects.value(FKey2Object.value(AKey)).fileOptions.value(AOption);
}

void FileStorage::loadDefinations(const QString &ADefFile, const QString APrefix)
{
  QDomDocument doc;
  QFile file(ADefFile);
  if (file.open(QFile::ReadOnly) && doc.setContent(file.readAll(),false))
  {
    QDomElement objElem = doc.documentElement().firstChildElement();
    while (!objElem.isNull())
    {
      if (FObjectTags.contains(objElem.tagName()))
      {
        StorageObject object;
        QDomElement keyElem = objElem.firstChildElement();
        while (!keyElem.isNull())
        {
          if (FKeyTags.contains(keyElem.tagName()))
          {
            QString key = keyElem.text();
            if (!FKey2Object.contains(key))
            {
              FKeys.append(key);
              FKey2Object.insert(key,FObjects.count());
            }
          }
          else if (FFileTags.contains(keyElem.tagName()))
          {
            if (!keyElem.text().isEmpty())
            {
              object.fileMimes.append(keyElem.attribute("mime"));
              object.fileNames.append(APrefix + keyElem.text());
            }
          }
          else if (!keyElem.hasChildNodes() && keyElem.attributes().count()==0)
          {
            object.fileOptions.insert(keyElem.tagName(),keyElem.text());
          }
          keyElem = keyElem.nextSiblingElement();
        }
        FObjects.append(object);
      }
      else if (objElem.firstChildElement().isNull() && objElem.attributes().count()==0 && !FOptions.contains(objElem.tagName()))
      {
        FOptions.insert(objElem.tagName(),objElem.text());
      }
      objElem = objElem.nextSiblingElement();
    }
    file.close();
  }
}

QStringList FileStorage::availStorages()
{
  QStringList storages;
  QDir dir(qApp->applicationDirPath());
  if (dir.cd(STORAGE_DIR))
  {
    storages = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    storages.removeAt(storages.indexOf(STORAGE_SHARED_DIR));
    for (QStringList::iterator it=storages.begin(); it!=storages.end(); it++)
    {
      if (dir.cd(*it))
      {
        if (dir.entryList(QStringList()<<STORAGE_DEFFILES_MASK).isEmpty())
          it = storages.erase(it);
        dir.cdUp();
      }
      else
        it = storages.erase(it);
    }
  }
  return storages;
}

QStringList FileStorage::availSubStorages(const QString &AStorage)
{
  QStringList storages;
  QDir dir(qApp->applicationDirPath());
  if (dir.cd(STORAGE_DIR) && dir.cd(AStorage))
  {
    storages = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    storages.removeAt(storages.indexOf(STORAGE_SHARED_DIR));
    QStringList::iterator it=storages.begin();
    while (it!=storages.end())
    {
      if (dir.cd(*it))
      {
        if (dir.entryList(QStringList()<<STORAGE_DEFFILES_MASK).isEmpty())
          it = storages.erase(it);
        else
          it++;
        dir.cdUp();
      }
      else
        it = storages.erase(it);
    }
  }
  return storages;
}

FileStorage *FileStorage::staticStorage(const QString &ASubStorage)
{
  FileStorage *storage = FStaticStorages.value(ASubStorage,NULL);
  if (!storage)
  {
    storage = new FileStorage(ASubStorage,"",qApp);
    FStaticStorages.insert(ASubStorage,storage);
  }
  return storage;
}
