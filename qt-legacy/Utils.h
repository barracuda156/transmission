/*
 * This file Copyright (C) 2009-2015 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 * $Id$
 */

#ifndef QTR_UTILS_H
#define QTR_UTILS_H

#include <cctype> // isxdigit()

// hidden from Qt 4's moc, which cannot parse C++20 standard headers
#ifndef Q_MOC_RUN
 #include <string>
 #include <string_view>
#endif

#include <QPointer>
#include <QRect>
#include <QString>

class QAbstractItemView;
class QColor;
class QHeaderView;
class QIcon;

class Utils
{
  public:
    // Qt 4's QString::fromStdString()/toStdString() convert through
    // Latin-1, so UTF-8 interchange with libtransmission must go
    // through fromUtf8()/toUtf8() explicitly.
    static QString toQString (const std::string& str)
    {
      return QString::fromUtf8 (str.data (), static_cast<int> (str.size ()));
    }
    static QString toQString (const std::string_view& str)
    {
      return QString::fromUtf8 (str.data (), static_cast<int> (str.size ()));
    }
    static std::string toStdUtf8 (const QString& str)
    {
      const QByteArray utf8 = str.toUtf8 ();
      return std::string (utf8.constData (), static_cast<size_t> (utf8.size ()));
    }

    static QIcon guessMimeIcon (const QString& filename);
    // Test if string is UTF-8 or not
    static bool isValidUtf8 (const char * s);

    static QString removeTrailingDirSeparator (const QString& path);

    static void narrowRect (QRect& rect, int dx1, int dx2, Qt::LayoutDirection direction)
    {
      if (direction == Qt::RightToLeft)
        qSwap (dx1, dx2);
      rect.adjust (dx1, 0, -dx2, 0);
    }

    static int measureViewItem (QAbstractItemView * view, const QString& text);
    static int measureHeaderItem (QHeaderView * view, const QString& text);

    static QColor getFadedColor (const QColor& color);

    template<typename DialogT, typename... ArgsT>
    static void
    openDialog (QPointer<DialogT>& dialog, ArgsT&&... args)
    {
      if (dialog.isNull ())
        {
          dialog = new DialogT (std::forward<ArgsT> (args)...);
          dialog->setAttribute (Qt::WA_DeleteOnClose);
          dialog->show ();
        }
      else
        {
          dialog->raise ();
          dialog->activateWindow ();
        }
    }

    ///
    /// URLs
    ///

    static bool isMagnetLink (const QString& s)
    {
      return s.startsWith (QString::fromUtf8 ("magnet:?"));
    }

    static bool isHexHashcode (const QString& s)
    {
      if (s.length() != 40)
        return false;
      for (const QChar ch: s) if (!isxdigit (ch.unicode())) return false;
      return true;
    }

    static bool isUriWithSupportedScheme (const QString& s)
    {
      static const QString ftp = QString::fromUtf8 ("ftp://");
      static const QString http = QString::fromUtf8 ("http://");
      static const QString https = QString::fromUtf8 ("https://");
      return s.startsWith(http) || s.startsWith(https) || s.startsWith(ftp);
    }
};

#endif // QTR_UTILS_H
