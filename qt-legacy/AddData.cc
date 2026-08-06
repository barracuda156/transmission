/*
 * This file Copyright (C) 2012-2015 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 * $Id$
 */

#include <string>
#include <string_view>

#include <QFile>
#include <QDir>

#include <libtransmission/transmission.h>
#include <libtransmission/crypto-utils.h> // tr_base64_encode()
#include <libtransmission/torrent-metainfo.h>

#include "AddData.h"
#include "Utils.h"

int
AddData::set (const QString& key)
{
  if (Utils::isMagnetLink (key))
    {
      magnet = key;
      type = MAGNET;
    }
  else if  (Utils::isUriWithSupportedScheme (key))
    {
      url = key;
      type = URL;
    }
  else if (QFile(key).exists ())
    {
      filename = QDir::fromNativeSeparators (key);
      type = FILENAME;

      QFile file (key);
      file.open (QIODevice::ReadOnly);
      metainfo = file.readAll ();
      file.close ();
    }
  else if (Utils::isHexHashcode (key))
    {
      magnet = QString::fromUtf8("magnet:?xt=urn:btih:") + key;
      type = MAGNET;
    }
  else
    {
      const QByteArray utf8 = key.toUtf8();
      const std::string raw = tr_base64_decode (std::string_view (utf8.constData(), static_cast<size_t> (utf8.size())));
      if (!raw.empty())
        {
          metainfo.append (raw.data(), int(raw.size()));
          type = METAINFO;
        }
      else
        {
          type = NONE;
        }
    }

  return type;
}

QByteArray
AddData::toBase64 () const
{
  QByteArray ret;

  if (!metainfo.isEmpty ())
    {
      const std::string b64 = tr_base64_encode (std::string_view (metainfo.constData(), static_cast<size_t> (metainfo.size())));
      ret = QByteArray (b64.data(), int(b64.size()));
    }

  return ret;
}

QString
AddData::readableName () const
{
  QString ret;

  switch (type)
    {
      case FILENAME:
        ret = filename;
        break;

      case MAGNET:
        ret = magnet;
        break;

      case URL:
        ret = url.toString();
        break;

      case METAINFO:
        {
          tr_torrent_metainfo tm;
          if (tm.parse_benc (std::string_view (metainfo.constData(), static_cast<size_t> (metainfo.size()))))
            ret = QString::fromStdString (tm.name ()); // metainfo is required to be UTF-8
          break;
        }
    }

  return ret;
}
