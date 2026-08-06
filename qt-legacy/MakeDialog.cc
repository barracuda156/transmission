/*
 * This file Copyright (C) 2009-2015 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 */

#include <cmath> // std::log2, std::pow

// moc processes this file for MakeProgressDialog below, and Qt 4's moc
// cannot parse the C++20 standard or libtransmission headers; it only
// needs the class declaration tokens, so hide the includes from it.
#ifndef Q_MOC_RUN
 #include <chrono>
 #include <future>
 #include <string>
 #include <utility>

 #include <libtransmission/transmission.h>
 #include <libtransmission/announce-list.h>
 #include <libtransmission/error.h>
 #include <libtransmission/makemeta.h>
#endif

#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QPushButton>
#include <QTimer>

#include "ColumnResizer.h"
#include "Formatter.h"
#include "MakeDialog.h"
#include "Session.h"
#include "Utils.h"

#include "ui_MakeProgressDialog.h"

namespace
{
  class MakeProgressDialog: public BaseDialog
  {
      Q_OBJECT

    public:
      MakeProgressDialog (Session& session, tr_metainfo_builder& builder, std::future<tr_error> future,
                          const QString& outfile, QWidget * parent = nullptr);

    private slots:
      void onButtonBoxClicked (QAbstractButton *);
      void onProgress ();

    private:
      Session& mySession;
      tr_metainfo_builder& myBuilder;
      std::future<tr_error> myFuture;
      const QString myOutfile;
      Ui::MakeProgressDialog ui;
      QTimer myTimer;
  };
}

MakeProgressDialog::MakeProgressDialog (Session& session, tr_metainfo_builder& builder,
                                        std::future<tr_error> future, const QString& outfile,
                                        QWidget * parent):
  BaseDialog (parent),
  mySession (session),
  myBuilder (builder),
  myFuture (std::move (future)),
  myOutfile (outfile)
{
  ui.setupUi (this);

  connect (ui.dialogButtons, SIGNAL (clicked (QAbstractButton *)),
           this, SLOT (onButtonBoxClicked (QAbstractButton *)));

  connect (&myTimer, SIGNAL (timeout ()), this, SLOT (onProgress ()));
  myTimer.start (100);

  onProgress ();
}

void
MakeProgressDialog::onButtonBoxClicked (QAbstractButton * button)
{
  switch (ui.dialogButtons->standardButton (button))
    {
      case QDialogButtonBox::Open:
        mySession.addNewlyCreatedTorrent (myOutfile,
                                          QFileInfo (QString::fromStdString (myBuilder.top ())).dir ().path ());
        break;

      case QDialogButtonBox::Abort:
        myBuilder.cancel_checksums ();
        break;

      default: // QDialogButtonBox::Ok:
        break;
    }

  close ();
}

void
MakeProgressDialog::onProgress ()
{
  // the checksum worker owns the future until it finishes; after the
  // first pass through the done branch below the future is spent, but
  // the timer is stopped then so this is not reached again
  const bool isDone = !myFuture.valid () ||
                      myFuture.wait_for (std::chrono::seconds (0)) == std::future_status::ready;

  if (isDone)
    myTimer.stop ();

  // progress bar
  int progress = 100; // [0..100]
  if (!isDone)
    {
      const std::pair<tr_piece_index_t, tr_piece_index_t> status = myBuilder.checksum_status ();
      const double denom = status.second != 0 ? status.second : 1;
      progress = static_cast<int> ((100.0 * status.first) / denom);
    }
  ui.progressBar->setValue (progress);

  // progress label
  const QString top = QString::fromStdString (myBuilder.top ());
  const QString base (QFileInfo (top).completeBaseName ());
  QString str;
  bool success = false;
  if (!isDone)
    {
      str = tr ("Creating \"%1\"").arg (base);
    }
  else
    {
      tr_error error = myFuture.get ();

      if (!error)
        myBuilder.save (myOutfile.toStdString (), &error);

      if (!error)
        {
          str = tr ("Created \"%1\"!").arg (base);
          success = true;
        }
      else
        {
          const QString errMsg = QString::fromUtf8 (error.message ().data (),
                                                    static_cast<int> (error.message ().size ()));
          str = tr ("Couldn't create \"%1\": %2 (%3)").arg (base).arg (errMsg).arg (error.code ());
        }
    }

  ui.progressLabel->setText (str);

  // buttons
  ui.dialogButtons->button (QDialogButtonBox::Abort)->setEnabled (!isDone);
  ui.dialogButtons->button (QDialogButtonBox::Ok)->setEnabled (isDone);
  ui.dialogButtons->button (QDialogButtonBox::Open)->setEnabled (isDone && success);
}

#include "MakeDialog.moc"

/***
****
***/

void
MakeDialog::makeTorrent ()
{
  if (myBuilder == nullptr)
    return;

  // get the announce list
  tr_announce_list trackers;
  trackers.parse (ui.trackersEdit->toPlainText ().toStdString ());
  myBuilder->set_announce_list (std::move (trackers));

  // the file to create
  const QString path = QString::fromStdString (myBuilder->top ());
  const QString torrentName = QFileInfo (path).completeBaseName () + QLatin1String (".torrent");
  const QString target = QDir (ui.destinationButton->path ()).filePath (torrentName);

  // comment
  if (ui.commentCheck->isChecked ())
    myBuilder->set_comment (ui.commentEdit->text ().toStdString ());

  // source
  if (ui.sourceCheck->isChecked ())
    myBuilder->set_source (ui.sourceEdit->text ().toStdString ());

  myBuilder->set_private (ui.privateCheck->isChecked ());

  // pop up the dialog
  MakeProgressDialog * dialog = new MakeProgressDialog (mySession, *myBuilder,
                                                        myBuilder->make_checksums (), target, this);
  dialog->setAttribute (Qt::WA_DeleteOnClose);
  dialog->open ();
}

/***
****
***/

QString
MakeDialog::getSource () const
{
  return (ui.sourceFileRadio->isChecked () ? ui.sourceFileButton : ui.sourceFolderButton)->path ();
}

/***
****
***/

void
MakeDialog::updatePiecesLabel ()
{
  QString text;

  if (myBuilder == nullptr)
    {
      text = tr ("<i>No source selected<i>");
      ui.pieceSizeSlider->setEnabled (false);
    }
  else
    {
      QString files = tr ("%Ln File(s)", 0, myBuilder->file_count ());
      QString pieces = tr ("%Ln Piece(s)", 0, myBuilder->piece_count ());
      text = tr ("%1 in %2; %3 @ %4")
               .arg (Formatter::sizeToString (myBuilder->total_size ()))
               .arg (files)
               .arg (pieces)
               .arg (Formatter::memToString (myBuilder->piece_size ()));
      ui.pieceSizeSlider->setEnabled (true);
    }

  ui.sourceSizeLabel->setText (text);
}

void
MakeDialog::onPieceSizeUpdated (int value)
{
  if (myBuilder != nullptr)
    myBuilder->set_piece_size (static_cast<uint32_t> (std::pow (2, value)));

  updatePiecesLabel ();
}

void
MakeDialog::onSourceChanged ()
{
  myBuilder.reset ();

  const QString filename = getSource ();
  if (!filename.isEmpty ())
    myBuilder.reset (new tr_metainfo_builder (filename.toStdString ()));

  updatePiecesLabel ();

  if (myBuilder != nullptr)
    ui.pieceSizeSlider->setValue (static_cast<int> (std::log2 (myBuilder->piece_size ())));
}

MakeDialog::MakeDialog (Session& session, QWidget * parent):
  BaseDialog (parent),
  mySession (session)
{
  ui.setupUi (this);

  ui.destinationButton->setMode (PathButton::DirectoryMode);
  ui.destinationButton->setPath (QDir::homePath ());

  ui.sourceFolderButton->setMode (PathButton::DirectoryMode);
  ui.sourceFileButton->setMode (PathButton::FileMode);

  ColumnResizer * cr (new ColumnResizer (this));
  cr->addLayout (ui.filesSectionLayout);
  cr->addLayout (ui.propertiesSectionLayout);
  cr->update ();

  resize (minimumSizeHint ());

  connect (ui.sourceFolderRadio, SIGNAL (toggled (bool)), this, SLOT (onSourceChanged ()));
  connect (ui.sourceFolderButton, SIGNAL (pathChanged (QString)), this, SLOT (onSourceChanged ()));
  connect (ui.sourceFileRadio, SIGNAL (toggled (bool)), this, SLOT (onSourceChanged ()));
  connect (ui.sourceFileButton, SIGNAL (pathChanged (QString)), this, SLOT (onSourceChanged ()));
  connect (ui.pieceSizeSlider, SIGNAL (valueChanged (int)), this, SLOT (onPieceSizeUpdated (int)));

  connect (ui.dialogButtons, SIGNAL (accepted ()), this, SLOT (makeTorrent ()));
  connect (ui.dialogButtons, SIGNAL (rejected ()), this, SLOT (close ()));

  onSourceChanged ();
}

MakeDialog::~MakeDialog ()
{
}

/***
****
***/

void
MakeDialog::dragEnterEvent (QDragEnterEvent * event)
{
  const QMimeData * mime = event->mimeData ();

  if (!mime->urls ().isEmpty () && QFileInfo (mime->urls ().front ().path ()).exists ())
    event->acceptProposedAction ();
}

void
MakeDialog::dropEvent (QDropEvent * event)
{
  const QString filename = event->mimeData ()->urls ().front ().path ();
  const QFileInfo fileInfo (filename);

  if (fileInfo.exists ())
    {
      if (fileInfo.isDir ())
        {
          ui.sourceFolderRadio->setChecked (true);
          ui.sourceFolderButton->setPath (filename);
        }
      else // it's a file
        {
          ui.sourceFileRadio->setChecked (true);
          ui.sourceFileButton->setPath (filename);
        }
    }
}
