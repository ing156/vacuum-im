#include "xmppstream.h"

#include <QTextDocument>

#define DISCONNECT_TIMEOUT          5000
#define KEEP_ALIVE_TIMEOUT          30000

XmppStream::XmppStream(IXmppStreams *AXmppStreams, const Jid &AStreamJid) : QObject(AXmppStreams->instance())
{
	FXmppStreams = AXmppStreams;

	FReady = false;
	FClosed = true;
	FEncrypt = true;
	FStreamJid = AStreamJid;
	FConnection = NULL;
	FStreamState = SS_OFFLINE;
	FPasswordDialog = NULL;

	connect(&FParser,SIGNAL(opened(QDomElement)), SLOT(onParserOpened(QDomElement)));
	connect(&FParser,SIGNAL(element(QDomElement)), SLOT(onParserElement(QDomElement)));
	connect(&FParser,SIGNAL(error(const QString &)), SLOT(onParserError(const QString &)));
	connect(&FParser,SIGNAL(closed()), SLOT(onParserClosed()));

	FKeepAliveTimer.setSingleShot(false);
	connect(&FKeepAliveTimer,SIGNAL(timeout()),SLOT(onKeepAliveTimeout()));
}

XmppStream::~XmppStream()
{
	abort(tr("XMPP stream destroyed"));
	clearActiveFeatures();
	emit streamDestroyed();
}

bool XmppStream::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream == this && AOrder == XSHO_XMPP_STREAM)
	{
		if (FStreamState==SS_INITIALIZE && AStanza.element().nodeName()=="stream:stream")
		{
			FStreamId = AStanza.id();
			setStreamState(SS_FEATURES);
			if (VersionParser(AStanza.element().attribute("version","0.0")) < VersionParser(1,0))
			{
				Stanza stanza("stream:features");
				stanza.addElement("register",NS_FEATURE_REGISTER);
				stanza.addElement("auth",NS_FEATURE_IQAUTH);
				xmppStanzaIn(AXmppStream, stanza, AOrder);
			}
			return true;
		}
		else if (FStreamState==SS_FEATURES && AStanza.element().nodeName()=="stream:features")
		{
			FServerFeatures = AStanza.element().cloneNode(true).toElement();
			FAvailFeatures = FXmppStreams->xmppFeatures();
			processFeatures();
			return true;
		}
		else if (AStanza.element().nodeName() == "stream:error")
		{
			ErrorHandler err(AStanza.element(),NS_XMPP_STREAMS);
			abort(err.message());
			return true;
		}
	}
	return false;
}

bool XmppStream::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

bool XmppStream::isOpen() const
{
	return FReady && !FClosed;
}

bool XmppStream::isConnected() const
{
	return FStreamState!=SS_OFFLINE;
}

bool XmppStream::open()
{
	if (FConnection && FStreamState==SS_OFFLINE)
	{
		FErrorString.clear();
		if (FConnection->connectToHost())
		{
			setStreamState(SS_CONNECTING);
			return true;
		}
		else
		{
			abort(tr("Failed to start connection"));
		}
	}
	else if (!FConnection)
	{
		emit error(tr("Connection not specified"));
	}
	return false;
}

void XmppStream::close()
{
	if (FConnection && FStreamState!=SS_OFFLINE && FStreamState!=SS_ERROR && FStreamState!=SS_DISCONNECTING)
	{
		setStreamState(SS_DISCONNECTING);
		if (FConnection->isOpen())
		{
			emit aboutToClose();
			sendData("</stream:stream>");
			FKeepAliveTimer.start(DISCONNECT_TIMEOUT);
			FClosed = true;
		}
		else
		{
			FClosed = true;
			FConnection->disconnectFromHost();
		}
	}
}

void XmppStream::abort(const QString &AError)
{
	if (FStreamState!=SS_OFFLINE && FStreamState!=SS_ERROR)
	{
		if (FStreamState != SS_DISCONNECTING)
		{
			setStreamState(SS_ERROR);
			FErrorString = AError;
			emit error(AError);
		}

		FClosed = true;
		FConnection->disconnectFromHost();
	}
}

QString XmppStream::streamId() const
{
	return FStreamId;
}

QString XmppStream::errorString() const
{
	return FErrorString;
}

Jid XmppStream::streamJid() const
{
	return FStreamJid;
}

void XmppStream::setStreamJid(const Jid &AJid)
{
	if (FStreamJid!=AJid && (FStreamState==SS_OFFLINE || FStreamState==SS_FEATURES))
	{
		if (FStreamState==SS_FEATURES && !FOfflineJid.isValid())
			FOfflineJid = FStreamJid;

		if (!(FStreamJid && AJid))
			FSessionPassword.clear();

		Jid before = FStreamJid;
		emit jidAboutToBeChanged(AJid);
		FStreamJid = AJid;
		emit jidChanged(before);
	}
}

QString XmppStream::password() const
{
	return FPassword;
}

void XmppStream::setPassword(const QString &APassword)
{
	if (FStreamState == SS_OFFLINE)
	{
		if (!APassword.isEmpty())
			FSessionPassword.clear();
		FPassword = APassword;
	}
}

QString XmppStream::getSessionPassword(bool AAskIfNeed)
{
	if (AAskIfNeed && FStreamState!=SS_ONLINE && FPassword.isEmpty() && FSessionPassword.isEmpty() && FPasswordMutex.tryLock())
	{
		bool isActive = isKeepAliveTimerActive();
		setKeepAliveTimerActive(false);

		FPasswordDialog = new QInputDialog(NULL,Qt::Dialog);
		FPasswordDialog->setWindowTitle(tr("Password request"));
		FPasswordDialog->setLabelText(tr("Enter password for <b>%1</b>").arg(Qt::escape(FStreamJid.bare())));
		FPasswordDialog->setTextEchoMode(QLineEdit::Password);
		if (FPasswordDialog->exec() == QDialog::Accepted)
			FSessionPassword = FPasswordDialog->textValue();
		FPasswordDialog->deleteLater();
		FPasswordDialog = NULL;

		setKeepAliveTimerActive(isActive);
		FPasswordMutex.unlock();
	}
	return !FSessionPassword.isEmpty() ? FSessionPassword : FPassword;
}

QString XmppStream::defaultLang() const
{
	return FDefLang;
}

void XmppStream::setDefaultLang(const QString &ADefLang)
{
	if (FStreamState == SS_OFFLINE)
		FDefLang = ADefLang;
}

bool XmppStream::isEncryptionRequired() const
{
	return FEncrypt;
}

void XmppStream::setEncryptionRequired(bool ARequire)
{
	if (FStreamState == SS_OFFLINE)
		FEncrypt = ARequire;
}

IConnection *XmppStream::connection() const
{
	return FConnection;
}

void XmppStream::setConnection(IConnection *AConnection)
{
	if (FStreamState==SS_OFFLINE && FConnection!=AConnection)
	{
		if (FConnection)
			FConnection->instance()->disconnect(this);

		if (AConnection)
		{
			connect(AConnection->instance(),SIGNAL(connected()),SLOT(onConnectionConnected()));
			connect(AConnection->instance(),SIGNAL(readyRead(qint64)),SLOT(onConnectionReadyRead(qint64)));
			connect(AConnection->instance(),SIGNAL(error(const QString &)),SLOT(onConnectionError(const QString &)));
			connect(AConnection->instance(),SIGNAL(disconnected()),SLOT(onConnectionDisconnected()));
		}

		FConnection = AConnection;
		emit connectionChanged(AConnection);
	}
}

bool XmppStream::isKeepAliveTimerActive() const
{
	return FKeepAliveTimer.isActive();
}

void XmppStream::setKeepAliveTimerActive(bool AActive)
{
	if (AActive && FStreamState!=SS_OFFLINE)
		FKeepAliveTimer.start(KEEP_ALIVE_TIMEOUT);
	else if (!AActive)
		FKeepAliveTimer.stop();
}

qint64 XmppStream::sendStanza(Stanza &AStanza)
{
	if (FStreamState!=SS_OFFLINE && FStreamState!=SS_ERROR)
	{
		if (!FClosed && !processStanzaHandlers(AStanza,true))
			return sendData(AStanza.toByteArray());
	}
	return -1;
}

void XmppStream::insertXmppDataHandler(int AOrder, IXmppDataHandler *AHandler)
{
	if (AHandler && !FDataHandlers.contains(AOrder, AHandler))
	{
		FDataHandlers.insertMulti(AOrder, AHandler);
		emit dataHandlerInserted(AOrder,AHandler);
	}
}

void XmppStream::removeXmppDataHandler(int AOrder, IXmppDataHandler *AHandler)
{
	if (FDataHandlers.contains(AOrder, AHandler))
	{
		FDataHandlers.remove(AOrder, AHandler);
		emit dataHandlerRemoved(AOrder,AHandler);
	}
}

void XmppStream::insertXmppStanzaHandler(int AOrder, IXmppStanzaHadler *AHandler)
{
	if (AHandler && !FStanzaHandlers.contains(AOrder, AHandler))
	{
		FStanzaHandlers.insertMulti(AOrder, AHandler);
		emit stanzaHandlerInserted(AOrder, AHandler);
	}
}

void XmppStream::removeXmppStanzaHandler(int AOrder, IXmppStanzaHadler *AHandler)
{
	if (FStanzaHandlers.contains(AOrder, AHandler))
	{
		FStanzaHandlers.remove(AOrder, AHandler);
		emit stanzaHandlerRemoved(AOrder, AHandler);
	}
}

void XmppStream::startStream()
{
	FParser.restart();
	setKeepAliveTimerActive(true);

	QDomDocument doc;
	QDomElement root = doc.createElementNS(NS_JABBER_STREAMS,"stream:stream");
	doc.appendChild(root);
	root.setAttribute("xmlns",NS_JABBER_CLIENT);
	root.setAttribute("to", FStreamJid.domain());
	if (!FDefLang.isEmpty())
		root.setAttribute("xml:lang",FDefLang);

	setStreamState(SS_INITIALIZE);
	Stanza stanza(doc.documentElement());
	if (!processStanzaHandlers(stanza,true))
	{
		QByteArray data = QString("<?xml version=\"1.0\"?>").toUtf8()+stanza.toByteArray().trimmed();
		data.remove(data.size()-2,1);
		sendData(data);
	}
}

void XmppStream::processFeatures()
{
	bool started = false;
	while (!started && !FAvailFeatures.isEmpty())
	{
		QString featureNS = FAvailFeatures.takeFirst();
		QDomElement featureElem = FServerFeatures.firstChildElement();
		while (!featureElem.isNull() && featureElem.namespaceURI()!=featureNS)
			featureElem = featureElem.nextSiblingElement();
		started = featureElem.namespaceURI()==featureNS ? startFeature(featureNS, featureElem) : false;
	}
	if (!started)
	{
		if (!isEncryptionRequired() || connection()->isEncrypted())
		{
			FReady = true;
			setStreamState(SS_ONLINE);
			emit opened();
		}
		else
		{
			abort(tr("Secure connection is not established"));
		}
	}
}

void XmppStream::clearActiveFeatures()
{
	foreach(IXmppFeature *feature, FActiveFeatures.toSet())
		delete feature->instance();
	FActiveFeatures.clear();
}

void XmppStream::setStreamState(StreamState AState)
{
	if (FPasswordDialog)
		FPasswordDialog->reject();
	FStreamState = AState;
}

bool XmppStream::startFeature(const QString &AFeatureNS, const QDomElement &AFeatureElem)
{
	foreach(IXmppFeaturesPlugin *plugin, FXmppStreams->xmppFeaturePlugins(AFeatureNS))
	{
		IXmppFeature *feature = plugin->newXmppFeature(AFeatureNS, this);
		if (feature && feature->start(AFeatureElem))
		{
			FActiveFeatures.append(feature);
			connect(feature->instance(),SIGNAL(finished(bool)),SLOT(onFeatureFinished(bool)));
			connect(feature->instance(),SIGNAL(error(const QString &)),SLOT(onFeatureError(const QString &)));
			connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
			connect(this,SIGNAL(closed()),feature->instance(),SLOT(deleteLater()));
			return true;
		}
		else if (feature)
		{
			feature->instance()->deleteLater();
		}
	}
	return false;
}

bool XmppStream::processDataHandlers(QByteArray &AData, bool ADataOut)
{
	bool hooked = false;
	QMapIterator<int, IXmppDataHandler *> it(FDataHandlers);
	if (!ADataOut)
		it.toBack();
	while (!hooked && (ADataOut ? it.hasNext() : it.hasPrevious()))
	{
		if (ADataOut)
		{
			it.next();
			hooked = it.value()->xmppDataOut(this, AData, it.key());
		}
		else
		{
			it.previous();
			hooked = it.value()->xmppDataIn(this, AData, it.key());
		}
	}
	return hooked;
}

bool XmppStream::processStanzaHandlers(Stanza &AStanza, bool AStanzaOut)
{
	bool hooked = false;
	QMapIterator<int, IXmppStanzaHadler *> it(FStanzaHandlers);
	if (!AStanzaOut)
	{
		if (AStanza.from().isEmpty() || Jid(FStreamJid.bare())==AStanza.from())
			AStanza.setFrom(FStreamJid.eFull());
		AStanza.setTo(FStreamJid.eFull());
		it.toBack();
	}
	while (!hooked && (AStanzaOut ? it.hasNext() : it.hasPrevious()))
	{
		if (AStanzaOut)
		{
			it.next();
			hooked = it.value()->xmppStanzaOut(this, AStanza, it.key());
		}
		else
		{
			it.previous();
			hooked = it.value()->xmppStanzaIn(this, AStanza, it.key());
		}
	}
	return hooked;
}

qint64 XmppStream::sendData(QByteArray AData)
{
	if (!processDataHandlers(AData,true))
	{
		setKeepAliveTimerActive(true);
		return FConnection->write(AData);
	}
	return 0;
}

QByteArray XmppStream::receiveData(qint64 ABytes)
{
	return FConnection->read(ABytes);
}

void XmppStream::onConnectionConnected()
{
	FClosed = false;
	insertXmppStanzaHandler(XSHO_XMPP_STREAM,this);
	startStream();
}

void XmppStream::onConnectionReadyRead(qint64 ABytes)
{
	QByteArray data = receiveData(ABytes);
	if (!processDataHandlers(data,false))
		if (!data.isEmpty())
			FParser.parseData(data);
}

void XmppStream::onConnectionError(const QString &AError)
{
	abort(AError);
}

void XmppStream::onConnectionDisconnected()
{
	FReady = false;
	FClosed = true;

	if (FStreamState != SS_DISCONNECTING)
		abort(tr("Connection closed unexpectedly"));

	setStreamState(SS_OFFLINE);
	setKeepAliveTimerActive(false);
	removeXmppStanzaHandler(XSHO_XMPP_STREAM,this);
	emit closed();

	clearActiveFeatures();
	if (FOfflineJid.isValid())
	{
		setStreamJid(FOfflineJid);
		FOfflineJid = Jid::null;
	}
}

void XmppStream::onParserOpened(QDomElement AElem)
{
	Stanza stanza(AElem);
	processStanzaHandlers(stanza,false);
}

void XmppStream::onParserElement(QDomElement AElem)
{
	Stanza stanza(AElem);
	processStanzaHandlers(stanza,false);
}

void XmppStream::onParserError(const QString &AError)
{
	static const QString xmlError(
		"<stream:error>"
			"<xml-not-well-formed xmlns='urn:ietf:params:xml:ns:xmpp-streams'/>"
			"<text xmlns='urn:ietf:params:xml:ns:xmpp-streams'>%1</text>"
		"</stream:error></stream:stream>");

	sendData(xmlError.arg(AError).toUtf8());
	abort(AError);
}

void XmppStream::onParserClosed()
{
	FClosed = true;
	FConnection->disconnectFromHost();
}

void XmppStream::onFeatureFinished(bool ARestart)
{
	if (!ARestart)
		processFeatures();
	else
		startStream();
}

void XmppStream::onFeatureError(const QString &AError)
{
	FSessionPassword.clear();
	abort(AError);
}

void XmppStream::onFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	FActiveFeatures.removeAll(feature);
}

void XmppStream::onKeepAliveTimeout()
{
	static const QByteArray space(1,' ');
	if (FStreamState == SS_DISCONNECTING)
		FConnection->disconnectFromHost();
	else if (FStreamState != SS_ONLINE)
		abort(tr("XMPP connection timed out"));
	else
		sendData(space);
}
