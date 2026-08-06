/*
 * This file Copyright (C) 2014-2015 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 */

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <QApplication>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <libtransmission/transmission.h>
#include <libtransmission/rpcimpl.h>
#include <libtransmission/version.h> // LONG_VERSION_STRING

#include "RpcClient.h"

// #define DEBUG_HTTP

#define REQUEST_DATA_PROPERTY_KEY "requestData"

namespace
{
  TrVariantPtr
  createVariant ()
  {
    return std::make_shared<tr_variant> ();
  }
}

RpcClient::RpcClient (QObject * parent):
  QObject (parent),
  mySession (nullptr),
  myNAM (nullptr)
{
  qRegisterMetaType<TrVariantPtr> ("TrVariantPtr");

  connect (this, SIGNAL (responseReceived (TrVariantPtr)),
           this, SLOT (parseResponse (TrVariantPtr)));
}

void
RpcClient::stop ()
{
  mySession = nullptr;
  mySessionId.clear ();
  myUrl.clear ();

  if (myNAM != nullptr)
    {
      myNAM->deleteLater ();
      myNAM = nullptr;
    }
}

void
RpcClient::start (tr_session * session)
{
  mySession = session;
}

void
RpcClient::start (const QUrl& url)
{
  myUrl = url;
}

bool
RpcClient::isLocal () const
{
  if (mySession != 0)
    return true;

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  if (myUrl.host () == QLatin1String ("127.0.0.1") ||
      myUrl.host ().compare (QLatin1String ("localhost"), Qt::CaseInsensitive) == 0)
    return true;
#else
  if (QHostAddress (myUrl.host ()).isLoopback ())
    return true;
#endif

  return false;
}

const QUrl&
RpcClient::url () const
{
  return myUrl;
}

void
RpcClient::exec (tr_quark method, tr_variant * args, int64_t tag)
{
  exec (std::string (tr_quark_get_string_view (method)).c_str (), args, tag);
}

void
RpcClient::exec (const char* method, tr_variant * args, int64_t tag)
{
  TrVariantPtr json = createVariant ();
  tr_variantInitDict (json.get (), 3);
  tr_variantDictAddStr (json.get (), TR_KEY_method, method);
  if (tag >= 0)
    tr_variantDictAddInt (json.get (), TR_KEY_tag, tag);
  if (args != nullptr)
    *tr_variantDictAdd (json.get (), TR_KEY_arguments) = std::move (*args);

  sendRequest (json);
}

void
RpcClient::sendRequest (TrVariantPtr json)
{
  if (mySession != nullptr)
    {
      // the response callback is invoked on the libtransmission session
      // thread; emitting responseReceived hands the response over to the
      // Qt thread through the queued connection made in the constructor
      tr_rpc_request_exec (mySession, *json,
        [this] (tr_session *, tr_variant&& response)
        {
          TrVariantPtr resp = createVariant ();
          *resp = std::move (response);
          emit responseReceived (resp);
        });
    }
  else if (!myUrl.isEmpty ())
    {
      QNetworkRequest request;
      request.setUrl (myUrl);
      request.setRawHeader ("User-Agent", (qApp->applicationName () + QLatin1Char ('/') + QString::fromUtf8 (LONG_VERSION_STRING)).toUtf8 ());
      request.setRawHeader ("Content-Type", "application/json; charset=UTF-8");

      if (!mySessionId.isEmpty ())
        request.setRawHeader (TR_RPC_SESSION_ID_HEADER, mySessionId.toUtf8 ());

      const std::string body = tr_variant_serde::json ().compact ().to_string (*json);
      QByteArray jsonData (body.data (), static_cast<int> (body.size ()));

      QNetworkReply * reply = networkAccessManager ()->post (request, jsonData);
      reply->setProperty (REQUEST_DATA_PROPERTY_KEY, QVariant::fromValue (json));
      connect (reply, SIGNAL (downloadProgress (qint64, qint64)), this, SIGNAL (dataReadProgress ()));
      connect (reply, SIGNAL (uploadProgress (qint64, qint64)), this, SIGNAL (dataSendProgress ()));
      connect (reply, SIGNAL (error (QNetworkReply::NetworkError)), this, SIGNAL (error (QNetworkReply::NetworkError)));

#ifdef DEBUG_HTTP
      std::cerr << "sending " << "POST " << qPrintable (myUrl.path ()) << std::endl;
      for (const QByteArray& b: request.rawHeaderList ())
        std::cerr << b.constData ()
                  << ": "
                  << request.rawHeader (b).constData ()
                  << std::endl;
      std::cerr << "Body:\n" << jsonData.constData () << std::endl;
#endif
    }
}

QNetworkAccessManager *
RpcClient::networkAccessManager ()
{
  if (myNAM == 0)
    {
      myNAM = new QNetworkAccessManager ();

      connect (myNAM, SIGNAL (finished (QNetworkReply *)),
               this, SLOT (onFinished (QNetworkReply *)));

      connect (myNAM, SIGNAL (authenticationRequired (QNetworkReply *,QAuthenticator *)),
               this, SIGNAL (httpAuthenticationRequired ()));
    }

  return myNAM;
}

void
RpcClient::onFinished (QNetworkReply * reply)
{
#ifdef DEBUG_HTTP
  std::cerr << "http response header: " << std::endl;
  for (const QByteArray& b: reply->rawHeaderList ())
    std::cerr << b.constData ()
              << ": "
              << reply->rawHeader (b).constData ()
              << std::endl;
  std::cerr << "json:\n" << reply->peek (reply->bytesAvailable ()).constData () << std::endl;
#endif

  if (reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt () == 409 &&
      reply->hasRawHeader (TR_RPC_SESSION_ID_HEADER))
    {
      // we got a 409 telling us our session id has expired.
      // update it and resubmit the request.
      mySessionId = QString::fromUtf8 (reply->rawHeader (TR_RPC_SESSION_ID_HEADER));
      sendRequest (reply->property (REQUEST_DATA_PROPERTY_KEY).value<TrVariantPtr> ());
    }
  else if (reply->error () != QNetworkReply::NoError)
    {
      emit errorMessage (reply->errorString ());
    }
  else
    {
      const QByteArray jsonData = reply->readAll ().trimmed ();

      std::optional<tr_variant> parsed = tr_variant_serde::json ().parse (
        std::string_view (jsonData.constData (), static_cast<size_t> (jsonData.size ())));
      if (parsed)
        {
          TrVariantPtr json = createVariant ();
          *json = std::move (*parsed);
          parseResponse (json);
        }

      emit error (QNetworkReply::NoError);
    }

  reply->deleteLater ();
}

void
RpcClient::parseResponse (TrVariantPtr json)
{
  int64_t tag;
  if (!tr_variantDictFindInt (json.get (), TR_KEY_tag, &tag))
    tag = -1;

  std::string_view result;
  const bool hasResult = tr_variantDictFindStrView (json.get (), TR_KEY_result, &result);

  tr_variant * args;
  if (!tr_variantDictFindDict (json.get (), TR_KEY_arguments, &args))
    args = nullptr;

  emit executed (tag,
                 hasResult ? QString::fromUtf8 (result.data (), static_cast<int> (result.size ())) : QString (),
                 args);
}
