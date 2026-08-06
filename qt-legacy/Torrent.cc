/*
 * This file Copyright (C) 2009-2015 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 * $Id$
 */

#include <cassert>
#include <iostream>
#include <string_view>

#include <QApplication>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QSet>
#include <QString>
#include <QStyle>
#include <QUrl>
#include <QVariant>

#include <libtransmission/transmission.h>
#include <libtransmission/quark.h>
#include <libtransmission/variant.h>

#include "Application.h"
#include "Prefs.h"
#include "Torrent.h"
#include "Utils.h"

Torrent::Torrent (const Prefs& prefs, int id):
  myPrefs (prefs),
  magnetTorrent (false)
{
#ifndef NDEBUG
  for (int i=0; i<PROPERTY_COUNT; ++i)
    assert (myProperties[i].id == i);
#endif

  setInt (ID, id);
  setIcon (MIME_ICON, qApp->style()->standardIcon (QStyle::SP_FileIcon));
}

Torrent::~Torrent ()
{
}

/***
****
***/

Torrent::Property
Torrent::myProperties[] =
{
  { ID, TR_KEY_id, QVariant::Int, INFO, },
  { UPLOAD_SPEED, TR_KEY_rate_upload_camel_APICOMPAT, QVariant::ULongLong, STAT } /* Bps */,
  { DOWNLOAD_SPEED, TR_KEY_rate_download_camel_APICOMPAT, QVariant::ULongLong, STAT }, /* Bps */
  { DOWNLOAD_DIR, TR_KEY_download_dir_camel_APICOMPAT, QVariant::String, STAT },
  { ACTIVITY, TR_KEY_status, QVariant::Int, STAT },
  { NAME, TR_KEY_name, QVariant::String, INFO },
  { ERROR, TR_KEY_error, QVariant::Int, STAT },
  { ERROR_STRING, TR_KEY_error_string_camel_APICOMPAT, QVariant::String, STAT },
  { SIZE_WHEN_DONE, TR_KEY_size_when_done_camel_APICOMPAT, QVariant::ULongLong, STAT },
  { LEFT_UNTIL_DONE, TR_KEY_left_until_done_camel_APICOMPAT, QVariant::ULongLong, STAT },
  { HAVE_UNCHECKED, TR_KEY_have_unchecked_camel_APICOMPAT, QVariant::ULongLong, STAT },
  { HAVE_VERIFIED, TR_KEY_have_valid_camel_APICOMPAT, QVariant::ULongLong, STAT },
  { DESIRED_AVAILABLE, TR_KEY_desired_available_camel_APICOMPAT, QVariant::ULongLong, STAT },
  { TOTAL_SIZE, TR_KEY_total_size_camel_APICOMPAT, QVariant::ULongLong, INFO },
  { PIECE_SIZE, TR_KEY_piece_size_camel_APICOMPAT, QVariant::ULongLong, INFO },
  { PIECE_COUNT, TR_KEY_piece_count_camel_APICOMPAT, QVariant::Int, INFO },
  { PEERS_GETTING_FROM_US, TR_KEY_peers_getting_from_us_camel_APICOMPAT, QVariant::Int, STAT },
  { PEERS_SENDING_TO_US, TR_KEY_peers_sending_to_us_camel_APICOMPAT, QVariant::Int, STAT },
  { WEBSEEDS_SENDING_TO_US, TR_KEY_webseeds_sending_to_us_camel_APICOMPAT, QVariant::Int, STAT_EXTRA },
  { PERCENT_DONE, TR_KEY_percent_done_camel_APICOMPAT, QVariant::Double, STAT },
  { METADATA_PERCENT_DONE, TR_KEY_metadata_percent_complete_camel_APICOMPAT, QVariant::Double, STAT },
  { PERCENT_VERIFIED, TR_KEY_recheck_progress_camel_APICOMPAT, QVariant::Double, STAT },
  { DATE_ACTIVITY, TR_KEY_activity_date_camel_APICOMPAT, QVariant::DateTime, STAT_EXTRA },
  { DATE_ADDED, TR_KEY_added_date_camel_APICOMPAT, QVariant::DateTime, INFO },
  { DATE_STARTED, TR_KEY_start_date_camel_APICOMPAT, QVariant::DateTime, STAT_EXTRA },
  { DATE_CREATED, TR_KEY_date_created_camel_APICOMPAT, QVariant::DateTime, INFO },
  { PEERS_CONNECTED, TR_KEY_peers_connected_camel_APICOMPAT, QVariant::Int, STAT },
  { ETA, TR_KEY_eta, QVariant::Int, STAT },
  { RATIO, TR_KEY_upload_ratio_camel_APICOMPAT, QVariant::Double, STAT },
  { DOWNLOADED_EVER, TR_KEY_downloaded_ever_camel_APICOMPAT, QVariant::ULongLong, STAT },
  { UPLOADED_EVER, TR_KEY_uploaded_ever_camel_APICOMPAT, QVariant::ULongLong, STAT },
  { FAILED_EVER, TR_KEY_corrupt_ever_camel_APICOMPAT, QVariant::ULongLong, STAT_EXTRA },
  { TRACKERS, TR_KEY_trackers, QVariant::StringList, STAT },
  { HOSTS, TR_KEY_NONE, QVariant::StringList, DERIVED },
  { LABELS, TR_KEY_labels, QVariant::StringList, STAT },
  { TRACKERSTATS, TR_KEY_tracker_stats_camel_APICOMPAT, CustomVariantType::TrackerStatsList, STAT_EXTRA },
  { MIME_ICON, TR_KEY_NONE, QVariant::Icon, DERIVED },
  { SEED_RATIO_LIMIT, TR_KEY_seed_ratio_limit_camel_APICOMPAT, QVariant::Double, STAT },
  { SEED_RATIO_MODE, TR_KEY_seed_ratio_mode_camel_APICOMPAT, QVariant::Int, STAT },
  { SEED_IDLE_LIMIT, TR_KEY_seed_idle_limit_camel_APICOMPAT, QVariant::Int, STAT_EXTRA },
  { SEED_IDLE_MODE, TR_KEY_seed_idle_mode_camel_APICOMPAT, QVariant::Int, STAT_EXTRA },
  { DOWN_LIMIT, TR_KEY_download_limit_camel_APICOMPAT, QVariant::Int, STAT_EXTRA }, /* KB/s */
  { DOWN_LIMITED, TR_KEY_download_limited_camel_APICOMPAT, QVariant::Bool, STAT_EXTRA },
  { UP_LIMIT, TR_KEY_upload_limit_camel_APICOMPAT, QVariant::Int, STAT_EXTRA }, /* KB/s */
  { UP_LIMITED, TR_KEY_upload_limited_camel_APICOMPAT, QVariant::Bool, STAT_EXTRA },
  { HONORS_SESSION_LIMITS, TR_KEY_honors_session_limits_camel_APICOMPAT, QVariant::Bool, STAT_EXTRA },
  { PEER_LIMIT, TR_KEY_peer_limit_kebab_APICOMPAT, QVariant::Int, STAT_EXTRA },
  { HASH_STRING, TR_KEY_hash_string_camel_APICOMPAT, QVariant::String, INFO },
  { IS_FINISHED, TR_KEY_is_finished_camel_APICOMPAT, QVariant::Bool, STAT },
  { IS_PRIVATE, TR_KEY_is_private_camel_APICOMPAT, QVariant::Bool, INFO },
  { IS_STALLED, TR_KEY_is_stalled_camel_APICOMPAT, QVariant::Bool, STAT },
  { COMMENT, TR_KEY_comment, QVariant::String, INFO },
  { CREATOR, TR_KEY_creator, QVariant::String, INFO },
  { MANUAL_ANNOUNCE_TIME, TR_KEY_manual_announce_time_camel_APICOMPAT, QVariant::DateTime, STAT_EXTRA },
  { PEERS, TR_KEY_peers, CustomVariantType::PeerList, STAT_EXTRA },
  { BANDWIDTH_PRIORITY, TR_KEY_bandwidth_priority_camel_APICOMPAT, QVariant::Int, STAT_EXTRA },
  { QUEUE_POSITION, TR_KEY_queue_position_camel_APICOMPAT, QVariant::Int, STAT },
};

Torrent::KeyList
Torrent::buildKeyList (Group group)
{
  KeyList keys;

  if (keys.empty())
    for (int i=0; i<PROPERTY_COUNT; ++i)
      if (myProperties[i].id==ID || myProperties[i].group==group)
        keys << myProperties[i].key;

  return keys;
}

const Torrent::KeyList&
Torrent::getInfoKeys ()
{
  static KeyList keys;

  if (keys.isEmpty())
    keys << buildKeyList(INFO) << TR_KEY_files;

  return keys;
}

const Torrent::KeyList&
Torrent::getStatKeys ()
{
  static KeyList keys (buildKeyList(STAT));
  return keys;
}

const Torrent::KeyList&
Torrent::getExtraStatKeys()
{
  static KeyList keys;

  if (keys.isEmpty())
    keys << buildKeyList(STAT_EXTRA) << TR_KEY_file_stats_camel_APICOMPAT;

  return keys;
}

bool
Torrent::setInt (int i, int value)
{
  bool changed = false;

  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::Int);

  if (myValues[i].isNull() || myValues[i].toInt()!=value)
    {
      myValues[i].setValue (value);
      changed = true;
    }

  return changed;
}

bool
Torrent::setBool (int i, bool value)
{
  bool changed = false;

  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::Bool);

  if (myValues[i].isNull() || myValues[i].toBool()!=value)
    {
      myValues[i].setValue (value);
      changed = true;
    }

  return changed;
}

bool
Torrent::setDouble (int i, double value)
{
  bool changed = false;

  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::Double);

  if (myValues[i] != value)
    {
      myValues[i].setValue (value);
      changed = true;
    }

  return changed;
}

bool
Torrent::setDateTime (int i, const QDateTime& value)
{
  bool changed = false;

  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::DateTime);

  if (myValues[i].isNull() || myValues[i].toDateTime()!=value)
    {
      myValues[i].setValue (value);
      changed = true;
    }

  return changed;
}

bool
Torrent::setSize (int i, qulonglong value)
{
  bool changed = false;

  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::ULongLong);

  if (myValues[i].isNull() || myValues[i].toULongLong()!=value)
    {
      myValues[i].setValue (value);
      changed = true;
    }

  return changed;
}

bool
Torrent::setString (int i, const QString& value)
{
  bool changed = false;

  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::String);

  if (myValues[i].isNull() || myValues[i].toString() != value)
    {
      myValues[i].setValue (value);
      changed = true;
    }

  return changed;
}

bool
Torrent::setIcon (int i, const QIcon& value)
{
  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::Icon);

  myValues[i].setValue (value);
  return true;
}

int
Torrent::getInt (int i) const
{
  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::Int);

  return myValues[i].toInt ();
}

QDateTime
Torrent::getDateTime (int i) const
{
  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::DateTime);

  return myValues[i].toDateTime ();
}

bool
Torrent::getBool (int i) const
{
  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::Bool);

  return myValues[i].toBool ();
}

qulonglong
Torrent::getSize (int i) const
{
    assert (0<=i && i<PROPERTY_COUNT);
    assert (myProperties[i].type == QVariant::ULongLong);

    return myValues[i].toULongLong ();
}
double
Torrent::getDouble (int i) const
{
  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::Double);

  return myValues[i].toDouble ();
}
QString
Torrent::getString (int i) const
{
  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::String);

  return myValues[i].toString ();
}
QIcon
Torrent::getIcon (int i) const
{
  assert (0<=i && i<PROPERTY_COUNT);
  assert (myProperties[i].type == QVariant::Icon);

  return myValues[i].value<QIcon>();
}

/***
****
***/

bool
Torrent::getSeedRatio (double& ratio) const
{
  bool isLimited;

  switch (seedRatioMode ())
    {
      case TR_RATIOLIMIT_SINGLE:
        isLimited = true;
        ratio = seedRatioLimit ();
        break;

      case TR_RATIOLIMIT_GLOBAL:
        if ((isLimited = myPrefs.getBool (Prefs::RATIO_ENABLED)))
          ratio = myPrefs.getDouble (Prefs::RATIO);
        break;

      default: // TR_RATIOLIMIT_UNLIMITED:
        isLimited = false;
        break;
    }

  return isLimited;
}

bool
Torrent::hasFileSubstring (const QString& substr) const
{
  for (const TorrentFile& file: myFiles)
    if (file.filename.contains (substr, Qt::CaseInsensitive))
      return true;

  return false;
}

bool
Torrent::hasTrackerSubstring (const QString& substr) const
{
  for (const QString& s: myValues[TRACKERS].toStringList())
    if (s.contains (substr, Qt::CaseInsensitive))
      return true;

  return false;
}

int
Torrent::compareSeedRatio (const Torrent& that) const
{
  double a;
  double b;
  const bool has_a = getSeedRatio (a);
  const bool has_b = that.getSeedRatio (b);
  if (!has_a && !has_b) return 0;
  if (!has_a || !has_b) return has_a ? -1 : 1;
  if (a < b) return -1;
  if (a > b) return 1;
  return 0;
}

int
Torrent::compareRatio (const Torrent& that) const
{
  const double a = ratio ();
  const double b = that.ratio ();
  if (static_cast<int> (a) == TR_RATIO_INF && static_cast<int> (b) == TR_RATIO_INF) return 0;
  if (static_cast<int> (a) == TR_RATIO_INF) return 1;
  if (static_cast<int> (b) == TR_RATIO_INF) return -1;
  if (a < b) return -1;
  if (a > b) return 1;
  return 0;
}

int
Torrent::compareETA (const Torrent& that) const
{
  const bool haveA (hasETA ());
  const bool haveB (that.hasETA ());
  if (haveA && haveB) return getETA() - that.getETA();
  if (haveA) return 1;
  if (haveB) return -1;
  return 0;
}

int
Torrent::compareTracker (const Torrent& that) const
{
  Q_UNUSED (that);

  // FIXME
  return 0;
}

/***
****
***/

void
Torrent::updateMimeIcon ()
{
  const FileList& files (myFiles);

  QIcon icon;

  if (files.size () > 1)
    icon = QFileIconProvider().icon (QFileIconProvider::Folder);
  else if (files.size () == 1)
    icon = Utils::guessMimeIcon (files.at(0).filename);
  else
    icon = QIcon ();

  setIcon (MIME_ICON, icon);
}

/***
****
***/

void
Torrent::notifyComplete () const
{
    // if someone wants to implement notification, here's the hook.
}

/***
****
***/

void
Torrent::update (tr_variant * d)
{
  static bool lookup_initialized = false;
  static int key_to_property_index[TR_N_KEYS];
  bool changed = false;
  const bool was_seed = isSeed ();
  const uint64_t old_verified_size = haveVerified ();

  if (!lookup_initialized)
    {
      lookup_initialized = true;
      for (int i=0; i<TR_N_KEYS; ++i)
        key_to_property_index[i] = -1;
      for (int i=0; i<PROPERTY_COUNT; i++)
        key_to_property_index[myProperties[i].key] = i;
    }

  tr_quark key;
  tr_variant * child;
  size_t pos = 0;
  while (tr_variantDictChild (d, pos++, &key, &child))
    {
      const int property_index = key_to_property_index[key];
      if (property_index == -1) // we're not interested in this one
        continue;

      assert (myProperties[property_index].key == key);

      switch (myProperties[property_index].type)
        {
          case QVariant::Int:
            {
              int64_t val;
              if (tr_variantGetInt (child, &val))
                changed |= setInt (property_index, val);
              break;
            }
          case QVariant::Bool:
            {
              bool val;
              if (tr_variantGetBool (child, &val))
                changed |= setBool (property_index, val);
              break;
            }
          case QVariant::String:
            {
              std::string_view val;
              if (tr_variantGetStrView (child, &val))
                changed |= setString (property_index, QString::fromUtf8 (val.data (), static_cast<int> (val.size ())));
              break;
            }
          case QVariant::ULongLong:
            {
              int64_t val;
              if (tr_variantGetInt (child, &val))
                changed |= setSize (property_index, val);
              break;
            }
          case QVariant::Double:
            {
              double val;
              if (tr_variantGetReal (child, &val))
                changed |= setDouble (property_index, val);
              break;
            }
          case QVariant::DateTime:
            {
              int64_t val;
              if (tr_variantGetInt (child, &val) && val)
                changed |= setDateTime (property_index, QDateTime::fromTime_t(val));
              break;
            }

          case QVariant::StringList:
          case CustomVariantType::PeerList:
            // handled below
            break;

          default:
            std::cerr << __FILE__ << ':' << __LINE__ << "unhandled type: " << tr_quark_get_string_view (key) << std::endl;
            assert (0 && "unhandled type");
        }
    }

  tr_variant * files;

  if (tr_variantDictFindList (d, TR_KEY_files, &files))
    {
      std::string_view str;
      int64_t intVal;
      int i = 0;
      tr_variant * child;

      myFiles.clear ();
      myFiles.reserve (tr_variantListSize (files));

      while ((child = tr_variantListChild (files, i)))
        {
          TorrentFile file;
          file.index = i++;

          if (tr_variantDictFindStrView (child, TR_KEY_name, &str))
            file.filename = QString::fromUtf8 (str.data (), static_cast<int> (str.size ()));
          if (tr_variantDictFindInt (child, TR_KEY_length, &intVal))
            file.size = intVal;

          myFiles.append (file);
        }

      updateMimeIcon ();
      changed = true;
    }

  if (tr_variantDictFindList (d, TR_KEY_file_stats_camel_APICOMPAT, &files))
    {
      const int n = tr_variantListSize (files);

      for (int i=0; i<n && i<myFiles.size(); ++i)
        {
          int64_t intVal;
          bool boolVal;
          tr_variant * child = tr_variantListChild (files, i);
          TorrentFile& file (myFiles[i]);

          if (tr_variantDictFindInt (child, TR_KEY_bytes_completed_camel_APICOMPAT, &intVal))
            file.have = intVal;
          if (tr_variantDictFindBool (child, TR_KEY_wanted, &boolVal))
            file.wanted = boolVal;
          if (tr_variantDictFindInt (child, TR_KEY_priority, &intVal))
            file.priority = intVal;
        }
        changed = true;
    }

  tr_variant * trackers;
  if (tr_variantDictFindList (d, TR_KEY_trackers, &trackers))
    {
      std::string_view str;
      int i = 0;
      QStringList list;
      tr_variant * child;

      while ((child = tr_variantListChild (trackers, i++)))
        {
          if (tr_variantDictFindStrView (child, TR_KEY_announce, &str))
            {
              const QString announce = QString::fromUtf8 (str.data (), static_cast<int> (str.size ()));
              qApp->faviconCache ().add (QUrl (announce));
              list.append (announce);
            }
        }

      if (myValues[TRACKERS] != list)
        {
          QStringList hosts;
          for (const QString& tracker: list)
            {
              const QString host = FaviconCache::getHost (QUrl (tracker));
              if (!host.isEmpty())
                hosts.append (host);
            }
          hosts.removeDuplicates();

          myValues[TRACKERS].setValue (list);
          myValues[HOSTS].setValue (hosts);
          changed = true;
        }
    }

  tr_variant * labels;
  if (tr_variantDictFindList (d, TR_KEY_labels, &labels))
    {
      std::string_view str;
      int i = 0;
      QStringList list;
      tr_variant * child;

      while ((child = tr_variantListChild (labels, i++)))
        {
          if (tr_variantGetStrView (child, &str))
            list.append (QString::fromUtf8 (str.data (), static_cast<int> (str.size ())));
        }

      if (myValues[LABELS] != list)
        {
          myValues[LABELS].setValue (list);
          changed = true;
        }
    }

  tr_variant * trackerStats;
  if (tr_variantDictFindList (d, TR_KEY_tracker_stats_camel_APICOMPAT, &trackerStats))
    {
      tr_variant * child;
      TrackerStatsList  trackerStatsList;
      int childNum = 0;

      while ((child = tr_variantListChild (trackerStats, childNum++)))
        {
          bool b;
          int64_t i;
          std::string_view str;
          TrackerStat trackerStat;

          if (tr_variantDictFindStrView (child, TR_KEY_announce, &str))
            {
              trackerStat.announce = QString::fromUtf8 (str.data (), static_cast<int> (str.size ()));
              qApp->faviconCache ().add (QUrl (trackerStat.announce));
            }

          if (tr_variantDictFindInt (child, TR_KEY_announce_state_camel_APICOMPAT, &i))
            trackerStat.announceState = i;
          if (tr_variantDictFindInt (child, TR_KEY_download_count_camel_APICOMPAT, &i))
            trackerStat.downloadCount = i;
          if (tr_variantDictFindBool (child, TR_KEY_has_announced_camel_APICOMPAT, &b))
            trackerStat.hasAnnounced = b;
          if (tr_variantDictFindBool (child, TR_KEY_has_scraped_camel_APICOMPAT, &b))
            trackerStat.hasScraped = b;
          if (tr_variantDictFindStrView (child, TR_KEY_host, &str))
            trackerStat.host = QString::fromUtf8 (str.data (), static_cast<int> (str.size ()));
          if (tr_variantDictFindInt (child, TR_KEY_id, &i))
            trackerStat.id = i;
          if (tr_variantDictFindBool (child, TR_KEY_is_backup_camel_APICOMPAT, &b))
            trackerStat.isBackup = b;
          if (tr_variantDictFindInt (child, TR_KEY_last_announce_peer_count_camel_APICOMPAT, &i))
            trackerStat.lastAnnouncePeerCount = i;
          if (tr_variantDictFindStrView (child, TR_KEY_last_announce_result_camel_APICOMPAT, &str))
            trackerStat.lastAnnounceResult = QString::fromUtf8 (str.data (), static_cast<int> (str.size ()));
          if (tr_variantDictFindInt (child, TR_KEY_last_announce_start_time_camel_APICOMPAT, &i))
            trackerStat.lastAnnounceStartTime = i;
          if (tr_variantDictFindBool (child, TR_KEY_last_announce_succeeded_camel_APICOMPAT, &b))
            trackerStat.lastAnnounceSucceeded = b;
          if (tr_variantDictFindInt (child, TR_KEY_last_announce_time_camel_APICOMPAT, &i))
            trackerStat.lastAnnounceTime = i;
          if (tr_variantDictFindBool (child, TR_KEY_last_announce_timed_out_camel_APICOMPAT, &b))
            trackerStat.lastAnnounceTimedOut = b;
          if (tr_variantDictFindStrView (child, TR_KEY_last_scrape_result_camel_APICOMPAT, &str))
            trackerStat.lastScrapeResult = QString::fromUtf8 (str.data (), static_cast<int> (str.size ()));
          if (tr_variantDictFindInt (child, TR_KEY_last_scrape_start_time_camel_APICOMPAT, &i))
            trackerStat.lastScrapeStartTime = i;
          if (tr_variantDictFindBool (child, TR_KEY_last_scrape_succeeded_camel_APICOMPAT, &b))
            trackerStat.lastScrapeSucceeded = b;
          if (tr_variantDictFindInt (child, TR_KEY_last_scrape_time_camel_APICOMPAT, &i))
            trackerStat.lastScrapeTime = i;
          if (tr_variantDictFindBool (child, TR_KEY_last_scrape_timed_out_camel_APICOMPAT, &b))
            trackerStat.lastScrapeTimedOut = b;
          if (tr_variantDictFindInt (child, TR_KEY_leecher_count_camel_APICOMPAT, &i))
            trackerStat.leecherCount = i;
          if (tr_variantDictFindInt (child, TR_KEY_next_announce_time_camel_APICOMPAT, &i))
            trackerStat.nextAnnounceTime = i;
          if (tr_variantDictFindInt (child, TR_KEY_next_scrape_time_camel_APICOMPAT, &i))
            trackerStat.nextScrapeTime = i;
          if (tr_variantDictFindInt (child, TR_KEY_scrape_state_camel_APICOMPAT, &i))
            trackerStat.scrapeState = i;
          if (tr_variantDictFindInt (child, TR_KEY_seeder_count_camel_APICOMPAT, &i))
            trackerStat.seederCount = i;
          if (tr_variantDictFindInt (child, TR_KEY_tier, &i))
            trackerStat.tier = i;

          trackerStatsList << trackerStat;
        }

      myValues[TRACKERSTATS].setValue (trackerStatsList);
      changed = true;
    }

  tr_variant * peers;
  if (tr_variantDictFindList (d, TR_KEY_peers, &peers))
    {
      tr_variant * child;
      PeerList peerList;
      int childNum = 0;
      while ((child = tr_variantListChild (peers, childNum++)))
        {
          double d;
          bool b;
          int64_t i;
          std::string_view str;
          Peer peer;

          if (tr_variantDictFindStrView (child, TR_KEY_address, &str))
            peer.address = QString::fromUtf8 (str.data (), static_cast<int> (str.size ()));
          if (tr_variantDictFindStrView (child, TR_KEY_client_name_camel_APICOMPAT, &str))
            peer.clientName = QString::fromUtf8 (str.data (), static_cast<int> (str.size ()));
          if (tr_variantDictFindBool (child, TR_KEY_client_is_choked_camel_APICOMPAT, &b))
            peer.clientIsChoked = b;
          if (tr_variantDictFindBool (child, TR_KEY_client_is_interested_camel_APICOMPAT, &b))
            peer.clientIsInterested = b;
          if (tr_variantDictFindStrView (child, TR_KEY_flag_str_camel_APICOMPAT, &str))
            peer.flagStr = QString::fromUtf8 (str.data (), static_cast<int> (str.size ()));
          if (tr_variantDictFindBool (child, TR_KEY_is_downloading_from_camel_APICOMPAT, &b))
            peer.isDownloadingFrom = b;
          if (tr_variantDictFindBool (child, TR_KEY_is_encrypted_camel_APICOMPAT, &b))
            peer.isEncrypted = b;
          if (tr_variantDictFindBool (child, TR_KEY_is_incoming_camel_APICOMPAT, &b))
            peer.isIncoming = b;
          if (tr_variantDictFindBool (child, TR_KEY_is_uploading_to_camel_APICOMPAT, &b))
            peer.isUploadingTo = b;
          if (tr_variantDictFindBool (child, TR_KEY_peer_is_choked_camel_APICOMPAT, &b))
            peer.peerIsChoked = b;
          if (tr_variantDictFindBool (child, TR_KEY_peer_is_interested_camel_APICOMPAT, &b))
            peer.peerIsInterested = b;
          if (tr_variantDictFindInt (child, TR_KEY_port, &i))
            peer.port = i;
          if (tr_variantDictFindReal (child, TR_KEY_progress, &d))
            peer.progress = d;
          if (tr_variantDictFindInt (child, TR_KEY_rate_to_client_camel_APICOMPAT, &i))
            peer.rateToClient = Speed::fromBps (i);
          if (tr_variantDictFindInt (child, TR_KEY_rate_to_peer_camel_APICOMPAT, &i))
            peer.rateToPeer = Speed::fromBps (i);
          peerList << peer;
        }

      myValues[PEERS].setValue (peerList);
      changed = true;
    }

  if (changed)
    emit torrentChanged (id ());

  if (!was_seed && isSeed() && (old_verified_size>0))
    emit torrentCompleted (id ());
}

QString
Torrent::activityString () const
{
  QString str;

  switch (getActivity ())
    {
      case TR_STATUS_STOPPED:       str = isFinished() ? tr("Finished"): tr("Paused"); break;
      case TR_STATUS_CHECK_WAIT:    str = tr("Queued for verification"); break;
      case TR_STATUS_CHECK:         str = tr("Verifying local data"); break;
      case TR_STATUS_DOWNLOAD_WAIT: str = tr("Queued for download"); break;
      case TR_STATUS_DOWNLOAD:      str = tr("Downloading"); break;
      case TR_STATUS_SEED_WAIT:     str = tr("Queued for seeding"); break;
      case TR_STATUS_SEED:          str = tr("Seeding"); break;
    }

  return str;
}

QString
Torrent::getError () const
{
  QString s = getString (ERROR_STRING);

  switch (getInt (ERROR))
    {
      case TR_STAT_TRACKER_WARNING: s = tr("Tracker gave a warning: %1").arg(s); break;
      case TR_STAT_TRACKER_ERROR: s = tr("Tracker gave an error: %1").arg(s); break;
      case TR_STAT_LOCAL_ERROR: s = tr("Error: %1").arg(s); break;
      default: s.clear(); break;
    }

  return s;
}

QPixmap
TrackerStat::getFavicon () const
{
  return qApp->faviconCache ().find (QUrl (announce));
}

