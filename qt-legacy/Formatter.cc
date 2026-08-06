/*
 * This file Copyright (C) 2012-2015 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 */

#include <string>
#include <string_view>

#include <libtransmission/transmission.h>
#include <libtransmission/utils.h> // tr_strpercent, tr_strratio
#include <libtransmission/values.h>

#include "Formatter.h"
#include "Speed.h"

namespace Values = libtransmission::Values;

/***
****  Constants
***/

namespace
{
  // the client-side Speed class stores B/s and converts to kB/s with
  // the same SI base that libtransmission's Values::Speed uses
  const unsigned int speed_K = 1000;
}

QString Formatter::unitStrings[3][5];

void
Formatter::initUnits ()
{
  // These tables feed unitStr() consumers such as the preferences and
  // details dialogs. The number formatting itself is done by
  // libtransmission's Values classes, whose bases match these labels
  // (speed and size use 1000, memory uses 1024).
  unitStrings[SPEED][B]  = tr ( "B/s");
  unitStrings[SPEED][KB] = tr ("kB/s");
  unitStrings[SPEED][MB] = tr ("MB/s");
  unitStrings[SPEED][GB] = tr ("GB/s");
  unitStrings[SPEED][TB] = tr ("TB/s");

  unitStrings[SIZE][B]  = tr ( "B");
  unitStrings[SIZE][KB] = tr ("kB");
  unitStrings[SIZE][MB] = tr ("MB");
  unitStrings[SIZE][GB] = tr ("GB");
  unitStrings[SIZE][TB] = tr ("TB");

  unitStrings[MEM][B]  = tr (  "B");
  unitStrings[MEM][KB] = tr ("KiB");
  unitStrings[MEM][MB] = tr ("MiB");
  unitStrings[MEM][GB] = tr ("GiB");
  unitStrings[MEM][TB] = tr ("TiB");
}

/***
****
***/

double
Speed::KBps () const
{
  return _Bps / static_cast<double> (speed_K);
}

Speed
Speed::fromKBps (double KBps)
{
  return static_cast<int> (KBps * speed_K);
}

/***
****
***/

QString
Formatter::memToString (int64_t bytes)
{
  if (bytes < 0)
    return tr ("Unknown");

  if (!bytes)
    return tr ("None");

  return QString::fromStdString (Values::Memory (bytes, Values::MemoryUnits::Bytes).to_string ());
}

QString
Formatter::sizeToString (int64_t bytes)
{
  if (bytes < 0)
    return tr ("Unknown");

  if (!bytes)
    return tr ("None");

  return QString::fromStdString (Values::Storage (bytes, Values::StorageUnits::Bytes).to_string ());
}

QString
Formatter::speedToString (const Speed& speed)
{
  return QString::fromStdString (Values::Speed (speed.Bps (), Values::SpeedUnits::Byps).to_string ());
}

QString
Formatter::uploadSpeedToString (const Speed& uploadSpeed)
{
  static const QChar uploadSymbol (0x25B4);

  return tr ("%1 %2").arg (speedToString (uploadSpeed)).arg (uploadSymbol);
}

QString
Formatter::downloadSpeedToString (const Speed& downloadSpeed)
{
  static const QChar downloadSymbol (0x25BE);

  return tr ("%1 %2").arg (speedToString (downloadSpeed)).arg (downloadSymbol);
}

QString
Formatter::percentToString (double x)
{
  return QString::fromStdString (tr_strpercent (x));
}

QString
Formatter::ratioToString (double ratio)
{
  static const std::string_view infinity = "\xE2\x88\x9E";

  return QString::fromStdString (tr_strratio (ratio, tr ("None").toStdString (), infinity));
}

QString
Formatter::timeToString (int seconds)
{
  int days, hours, minutes;
  QString d, h, m, s;
  QString str;

  if (seconds < 0)
    seconds = 0;

  days = seconds / 86400;
  hours = (seconds % 86400) / 3600;
  minutes = (seconds % 3600) / 60;
  seconds %= 60;

  d = tr ("%Ln day(s)", 0, days);
  h = tr ("%Ln hour(s)", 0, hours);
  m = tr ("%Ln minute(s)", 0, minutes);
  s = tr ("%Ln second(s)", 0, seconds);

  if (days)
    {
      if (days >= 4 || !hours)
        str = d;
      else
        str = tr ("%1, %2").arg (d).arg (h);
    }
  else if (hours)
    {
      if (hours >= 4 || !minutes)
        str = h;
      else
        str = tr ("%1, %2").arg (h).arg (m);
    }
  else if (minutes)
    {
      if (minutes >= 4 || !seconds)
        str = m;
      else
        str = tr ("%1, %2").arg (m).arg (s);
    }
  else
    {
      str = s;
    }

  return str;
}
