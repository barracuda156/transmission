/*
 * This file Copyright (C) 2009-2015 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 */

#ifndef QTR_MAKE_DIALOG_H
#define QTR_MAKE_DIALOG_H

#include <memory>

#include "BaseDialog.h"

#include "ui_MakeDialog.h"

class QAbstractButton;

class Session;

// the builder is only referenced through a pointer here, so a bare
// declaration keeps the C++20 libtransmission headers out of this
// moc'd header
class tr_metainfo_builder;

class MakeDialog: public BaseDialog
{
    Q_OBJECT

  public:
    MakeDialog (Session&, QWidget * parent = nullptr);
    virtual ~MakeDialog ();

  protected:
    // QWidget
    virtual void dragEnterEvent (QDragEnterEvent *);
    virtual void dropEvent (QDropEvent *);

  private:
    QString getSource () const;
    void updatePiecesLabel ();

  private slots:
    void onSourceChanged ();
    void onPieceSizeUpdated (int value);
    void makeTorrent ();

  private:
    Session& mySession;

    Ui::MakeDialog ui;

    std::unique_ptr<tr_metainfo_builder> myBuilder;
};

#endif // QTR_MAKE_DIALOG_H
