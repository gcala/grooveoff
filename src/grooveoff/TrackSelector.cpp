/*
    GrooveOff - Music streaming from torrents
    Copyright (C) 2026  Giuseppe Calà <jiveaxe@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "TrackSelector.h"
#include "ui_TrackSelector.h"
#include "Roles.h"

#include <QHeaderView>
#include <QLocale>
#include <QPushButton>
#include <QTreeWidgetItem>

namespace {

/** the row keeps the track it stands for, so no lookup is needed later */
const int trackRole = Qt::UserRole + 1;

} // namespace

TrackSelector::TrackSelector( const QString &torrentName,
                              const QList<TrackInfoPtr> &tracks,
                              const QList<qint64> &chosen,
                              QWidget *parent )
    : QDialog( parent )
    , ui( new Ui::TrackSelector )
    , m_tracks( tracks )
    , m_chosen( chosen )
    , m_hasAudio( false )
{
    ui->setupUi( this );

    // The last column stretches by default, which here gave almost the whole
    // dialog to four digits and a unit while the file names — the one thing
    // being chosen between — were elided down to "Carpent…". A size needs the
    // width of a size; everything left over belongs to the name.
    QHeaderView *header = ui->fileList->header();
    header->setStretchLastSection( false );
    header->setSectionResizeMode( 0, QHeaderView::Stretch );
    header->setSectionResizeMode( 1, QHeaderView::ResizeToContents );

    ui->headerLabel->setText( torrentName );
    ui->filterLine->setPlaceholderText( tr( "Filter" ) );
    ui->filterLine->setClearButtonEnabled( true );

    for( const TrackInfoPtr &track : m_tracks ) {
        if( track->isAudio() ) {
            m_hasAudio = true;
            break;
        }
    }

    // A torrent with nothing recognisable in it opens showing everything, or
    // it would open empty and say nothing about why.
    if( !m_hasAudio ) {
        ui->showAllFiles->setChecked( true );
        ui->headerLabel->setText( torrentName + QLatin1String( "\n" )
                                  + tr( "No file of this torrent has a known audio extension." ) );
    }

    fill();

    connect( ui->filterLine,       &QLineEdit::textChanged,   this, &TrackSelector::applyFilter );
    connect( ui->showAllFiles,     &QCheckBox::toggled,       this, &TrackSelector::toggleAllFiles );
    connect( ui->selectAllButton,  &QPushButton::clicked,     this, &TrackSelector::selectAll );
    connect( ui->selectNoneButton, &QPushButton::clicked,     this, &TrackSelector::selectNone );
    connect( ui->fileList,         &QTreeWidget::itemChanged, this, &TrackSelector::updateCount );

    updateCount();
}

TrackSelector::~TrackSelector()
{
    delete ui;
}

void TrackSelector::fill()
{
    ui->fileList->clear();

    const bool isNewTorrent = m_chosen.isEmpty();

    for( const TrackInfoPtr &track : std::as_const( m_tracks ) ) {
        QTreeWidgetItem *item = new QTreeWidgetItem( ui->fileList );

        item->setText( 0, track->filePath() );
        item->setText( 1, QLocale().formattedDataSize( track->fileSize() ) );
        item->setToolTip( 0, track->filePath() );
        item->setData( 0, trackRole, QVariant::fromValue( track ) );

        // Everything ticked for a torrent nobody has chosen from yet; what is
        // in the playlist for one that has been seen before. A file left out
        // last time is simply unticked — there is no refusal kept anywhere, so
        // «Select all» really does select all.
        const bool wanted = isNewTorrent ? track->isAudio() : m_chosen.contains( track->id() );

        item->setCheckState( 0, wanted ? Qt::Checked : Qt::Unchecked );

        item->setHidden( !isVisible( item ) );
    }
}

bool TrackSelector::isVisible( const QTreeWidgetItem *item ) const
{
    const TrackInfoPtr track = item->data( 0, trackRole ).value<TrackInfoPtr>();
    if( !track )
        return false;

    if( !track->isAudio() && !ui->showAllFiles->isChecked() )
        return false;

    const QString filter = ui->filterLine->text().trimmed();
    if( filter.isEmpty() )
        return true;

    return track->filePath().contains( filter, Qt::CaseInsensitive );
}

void TrackSelector::applyFilter( const QString &text )
{
    Q_UNUSED( text )

    for( int i = 0; i < ui->fileList->topLevelItemCount(); ++i ) {
        QTreeWidgetItem *item = ui->fileList->topLevelItem( i );
        item->setHidden( !isVisible( item ) );
    }
}

void TrackSelector::toggleAllFiles( bool showAll )
{
    Q_UNUSED( showAll )

    applyFilter( ui->filterLine->text() );
}

void TrackSelector::selectAll()
{
    // Only what is on screen. Pressing select all with a filter typed in and
    // getting three hundred files back would be a surprise, and a bad one.
    for( int i = 0; i < ui->fileList->topLevelItemCount(); ++i ) {
        QTreeWidgetItem *item = ui->fileList->topLevelItem( i );
        if( !item->isHidden() )
            item->setCheckState( 0, Qt::Checked );
    }
}

void TrackSelector::selectNone()
{
    for( int i = 0; i < ui->fileList->topLevelItemCount(); ++i ) {
        QTreeWidgetItem *item = ui->fileList->topLevelItem( i );
        if( !item->isHidden() )
            item->setCheckState( 0, Qt::Unchecked );
    }
}

void TrackSelector::updateCount()
{
    const QList<TrackInfoPtr> selected = selectedTracks();

    qint64 bytes = 0;
    for( const TrackInfoPtr &track : selected )
        bytes += track->fileSize();

    ui->countLabel->setText( tr( "%n track(s)", "", int( selected.count() ) )
                             + QLatin1String( ", " )
                             + QLocale().formattedDataSize( bytes ) );

    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( !selected.isEmpty() );
}

QList<TrackInfoPtr> TrackSelector::selectedTracks() const
{
    QList<TrackInfoPtr> selected;

    for( int i = 0; i < ui->fileList->topLevelItemCount(); ++i ) {
        const QTreeWidgetItem *item = ui->fileList->topLevelItem( i );
        if( item->checkState( 0 ) != Qt::Checked )
            continue;

        const TrackInfoPtr track = item->data( 0, trackRole ).value<TrackInfoPtr>();
        if( track )
            selected << track;
    }

    return selected;
}

