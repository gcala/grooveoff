/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>

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


#include "grooveoff/audioplayer.h"
#include "ui_audioplayer.h"
#include "grooveoff/utility.h"

#include <QDir>
#include <QTime>
#include <QGraphicsDropShadowEffect>

/*!
  \brief AudioPlayer: this is the AudioPlayer constructor.
  \param parent: The Parent Widget
*/
AudioPlayer::AudioPlayer(QWidget *parent) :
    QWidget(parent),
    ui_(new Ui::AudioPlayer)
{
    ui_->setupUi(this);
    setupUi();

    oldIndex_ = -1;

    // The AudioOutput class is used to send data to audio output devices
    audioOutput_ = new Phonon::AudioOutput(Phonon::MusicCategory, this);

    // The media object knows how to playback multimedia
    mediaObject_ = new Phonon::MediaObject(this);

    // Get the meta information from the music files
    metaInformationResolver_ = new Phonon::MediaObject(this);

    // Supply with the MediaObject object seekSlider should control
    ui_->seekSlider->setMediaObject(mediaObject_);

    mediaObject_->setTickInterval(1000);

    // Phonon is a graph based framework, i.e., its objects are nodes that
    // can be connected by paths. Objects are connected using the createPath()
    // function, which is part of the Phonon namespace.
    Phonon::createPath(mediaObject_, audioOutput_);

    connect(mediaObject_, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
            this, SLOT(stateChanged(Phonon::State,Phonon::State)));

    connect(mediaObject_, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));

    connect(mediaObject_, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));

    connect(mediaObject_,SIGNAL(currentSourceChanged(Phonon::MediaSource)), this, SLOT(sourceChanged(Phonon::MediaSource)));

    connect(ui_->timeLabel, SIGNAL(clicked()), this, SLOT(toggleTimeLabel()));

    updateState_ = true;
}

/*!
  \brief ~AudioPlayer: this is the AudioPlayer destructor.
*/
AudioPlayer::~AudioPlayer()
{
    mediaObject_->stop();
}

/*!
  \brief setupUi: setup ui elements
  \return void
*/
void AudioPlayer::setupUi()
{
    QGraphicsDropShadowEffect *coverShadow = new QGraphicsDropShadowEffect(this);
    coverShadow->setBlurRadius(10.0);
    coverShadow->setColor(palette().color(QPalette::Shadow));
    coverShadow->setOffset(0.0);

    ui_->coverLabel->setWindowFlags(Qt::FramelessWindowHint);
    ui_->coverLabel->setAttribute(Qt::WA_TranslucentBackground);
    ui_->coverLabel->setGraphicsEffect(coverShadow);

    ui_->coverLabel->setScaledContents(true);
    ui_->titleLabel->setFont(Utility::font(QFont::Bold));
    ui_->album_authorLabel->setFont(Utility::font(QFont::Bold));
    ui_->stackedWidget->setBackgroundRole(QPalette::Base);
    ui_->messageLabel->setFont(Utility::font(QFont::Bold,1));

    ui_->previousButton->setIcon(QIcon::fromTheme(QLatin1String("media-seek-backward"), QIcon(QLatin1String(":/resources/media-seek-backward.png"))));
    ui_->previousButton->setFlat(true);
    ui_->previousButton->setStyleSheet("border: none; outline: none;");
    ui_->previousButton->setToolTip(trUtf8("Play Previous"));
    ui_->previousButton->setFixedSize(QSize(16,16));
    ui_->previousButton->setIconSize(QSize(16,16));
    ui_->previousButton->setEnabled(false);
    connect(ui_->previousButton, SIGNAL(clicked(bool)), this, SLOT(playPrevious()));

    ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"), QIcon(QLatin1String(":/resources/media-playback-start.png"))));
    ui_->playPauseButton->setFlat(true);
    ui_->playPauseButton->setStyleSheet("border: none; outline: none;");
    ui_->playPauseButton->setToolTip(trUtf8("Play"));
    ui_->playPauseButton->setFixedSize(QSize(32,32));
    ui_->playPauseButton->setIconSize(QSize(32,32));
    ui_->playPauseButton->setEnabled(false);
    connect(ui_->playPauseButton, SIGNAL(clicked(bool)), this, SLOT(pauseResumePlaying()));

    ui_->stopButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-stop"), QIcon(QLatin1String(":/resources/media-playback-stop.png"))));
    ui_->stopButton->setToolTip(trUtf8("Stop "));
    ui_->stopButton->setFlat(true);
    ui_->stopButton->setStyleSheet("border: none; outline: none;");
    ui_->stopButton->setFixedSize(QSize(24,24));
    ui_->stopButton->setIconSize(QSize(24,24));
    ui_->stopButton->setContentsMargins( 0, 0, 0, 0 );
    connect(ui_->stopButton, SIGNAL(clicked(bool)), this, SLOT(stopPlaying()));

    ui_->nextButton->setIcon(QIcon::fromTheme(QLatin1String("media-seek-forward"), QIcon(QLatin1String(":/resources/media-seek-forward.png"))));
    ui_->nextButton->setFlat(true);
    ui_->nextButton->setStyleSheet("border: none; outline: none;");
    ui_->nextButton->setToolTip(trUtf8("Play Next"));
    ui_->nextButton->setFixedSize(QSize(16,16));
    ui_->nextButton->setIconSize(QSize(16,16));
    ui_->nextButton->setEnabled(false);
    connect(ui_->nextButton, SIGNAL(clicked(bool)), this, SLOT(playNext()));

    ui_->timeLabel->setText("00:00");
    ui_->timeLabel->setMinimumSize(QSize(50,0));
    ui_->timeLabel->setFont(Utility::monoFont());

    ui_->elapsedTimeLabel->setText("00:00");
    ui_->elapsedTimeLabel->setMinimumSize(QSize(50,0));
    ui_->elapsedTimeLabel->setFont(Utility::monoFont());
}

/*!
  \brief showElapsedTimerLabel: show/hide elapsedTimeLabel
  \param ok: bool
  \return void
*/
void AudioPlayer::showElapsedTimerLabel(bool ok)
{
    ui_->elapsedTimeLabel->setVisible(ok);
}

/*!
  \brief stateChanged: monitor state changes
  \param newState: new state
  \param oldState: old state
  \return void
*/
void AudioPlayer::stateChanged(Phonon::State newState, Phonon::State oldState)
{
    Q_UNUSED(oldState)

    int index = Utility::audioSources.indexOf(mediaObject_->currentSource());

    switch (newState) {
    case Phonon::ErrorState:
        if (mediaObject_->errorType() == Phonon::FatalError) {
            qDebug() << "GrooveOff :: "  << "Fatal Error: " << mediaObject_->errorString();
        } else {
            qDebug() << "GrooveOff :: "  << "Error: " << mediaObject_->errorString();
        }
        break;
    case Phonon::PlayingState:
        //ui_->labelsContainerWidget->setVisible(true);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-pause"),
                                      QIcon(QLatin1String(":/resources/media-playback-pause.png"))));
        Utility::playlist.at(index).data()->setPlayerState(Phonon::PlayingState);
        break;
    case Phonon::StoppedState:
        //ui_->labelsContainerWidget->setVisible(false);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"),
                                      QIcon(QLatin1String(":/resources/media-playback-start.png"))));
        ui_->timeLabel->setText("00:00");
        ui_->elapsedTimeLabel->setText("00:00");
        Utility::playlist.at(index).data()->setPlayerState(Phonon::StoppedState);
        break;
    case Phonon::PausedState:
        //ui_->labelsContainerWidget->setVisible(true);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"),
                                      QIcon(QLatin1String(":/resources/media-playback-start.png"))));
        Utility::playlist.at(index).data()->setPlayerState(Phonon::PausedState);
        break;
    default:
        /* do nothing */
        break;
    }
}

/*!
  \brief tick: change in playback position
  \param time: current play time
  \return void
*/
void AudioPlayer::tick(qint64 elapsedTime)
{
    if(mediaObject_->state() == Phonon::StoppedState)
        return;

    quint64 remainingTime = mediaObject_->remainingTime();

    if(ui_->elapsedTimeLabel->isVisible()) {
        ui_->elapsedTimeLabel->setText(QTime(0, (elapsedTime / 60000) % 60, (elapsedTime / 1000) % 60).toString("mm:ss"));
        ui_->timeLabel->setText("-" + QTime(0, (remainingTime / 60000) % 60, (remainingTime / 1000) % 60).toString("mm:ss"));
    } else {
        switch(timerState) {
            case GrooveOff::ElapsedState:
                ui_->timeLabel->setText(QTime(0, (elapsedTime / 60000) % 60, (elapsedTime / 1000) % 60).toString("mm:ss"));
                break;
            case GrooveOff::RemainingState:
                ui_->timeLabel->setText("-" + QTime(0, (remainingTime / 60000) % 60, (remainingTime / 1000) % 60).toString("mm:ss"));
                break;
        }
    }
}

/*!
  \brief aboutToFinish: guaranteed to be emitted while there is still time to enqueue another file for playback.
  \return void
*/
void AudioPlayer::aboutToFinish()
{
    int index = Utility::audioSources.indexOf(mediaObject_->currentSource()) + 1;
    if (Utility::audioSources.size() > index) {
        mediaObject_->enqueue(Utility::audioSources.at(index));
    }
}

/*!
  \brief stopPlaying: stop playing
  \return void
*/
void AudioPlayer::stopPlaying()
{
    mediaObject_->stop();
}

/*!
  \brief pauseResumePlaying: controls pause/resume
  \return void
*/
void AudioPlayer::pauseResumePlaying()
{
    if(mediaObject_->state() == Phonon::PlayingState) {
        mediaObject_->pause();
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"), QIcon(QLatin1String(":/resources/media-playback-start.png"))));
        ui_->playPauseButton->setToolTip(trUtf8("Play"));
    } else{
        mediaObject_->play();
        //ui_->labelsContainerWidget->setVisible(true);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-pause"), QIcon(QLatin1String(":/resources/media-playback-pause.png"))));
        ui_->playPauseButton->setToolTip(trUtf8("Pause"));
    }
}

void AudioPlayer::toggleTimeLabel()
{
    timerState = (GrooveOff::TimerState)!timerState;
}

void AudioPlayer::showMessage(const QString& message)
{
    ui_->stackedWidget->setCurrentIndex(0);
    ui_->messageLabel->setText(message);
}

void AudioPlayer::removeFromPlaylist()
{
    for(int i = 0; i < Utility::playlist.count(); i++) {
        if(Utility::playlist.at(i)->info()->songID() == ((SongItem *)QObject::sender())->info()->songID()) {
            bool isPlaying = false;
            qDebug() << "indice canzone rimuovere" << i;
            qDebug() << "oldIndex_" << oldIndex_;
            if(i == oldIndex_) {
                qDebug() << "sto per fermare";
                mediaObject_->stop();
                isPlaying = true;
            }
            qDebug() << "fermato";
            Utility::playlist.removeAt(i);
            qDebug() << "rimosso da playlist";
            Utility::audioSources.removeAt(i);
            qDebug() << "rimosso da audio sources";
            oldIndex_--;
            qDebug() << "oldIndex_" << oldIndex_;
            if(isPlaying) {
                oldIndex_++;
                qDebug() << "oldIndex_" << oldIndex_;
                //play(oldIndex_);
            }
            break;
        }
    }
}

void AudioPlayer::sourceChanged(Phonon::MediaSource newSource)
{
    // fermare la riproduzione precedente
    emit cambioStato(Phonon::StoppedState, oldSource_.url().toString());

    // recuperiamo l'indice della sorgente attuale per compilare i label
    int index = currentIndex(newSource.url().toString());
    setupLabels(index);

    // la nuiva sorgente viene flaggata
    oldSource_ = newSource;

    // comunichiamo il cambio di stato
    emit cambioStato(mediaObject_->state(), newSource.url().toString());
}

void AudioPlayer::setupLabels(int index)
{
    QString title = Utility::playlist.at(index)->info()->songName();
    QString artist = Utility::playlist.at(index)->info()->artistName();
    QString album = Utility::playlist.at(index)->info()->albumName();
    QString path = Utility::playlist.at(index)->path();
    QString coverName = Utility::playlist.at(index)->info()->coverArtFilename();

    ui_->titleLabel->setText(title);
    ui_->titleLabel->setToolTip(title);

    ui_->album_authorLabel->setText(artist + " - " + album);

    if(!coverName.isEmpty() && QFile::exists(Utility::coversCachePath + coverName))
        ui_->coverLabel->setPixmap(QPixmap(Utility::coversCachePath + coverName));
    else
        ui_->coverLabel->setPixmap(QIcon::fromTheme(QLatin1String("media-optical"),
                                   QIcon(QLatin1String(":/resources/media-optical.png"))).pixmap(ui_->coverLabel->size()));
}

void AudioPlayer::play(QString source)
{
    int index = currentIndex(source);

    if(index < 0)
        return;

//    metaInformationResolver_->setCurrentSource(Utility::audioSources.at(index));
    mediaObject_->setCurrentSource(Utility::audioSources.at(index));
    mediaObject_->play();
    setupLabels(index);
    ui_->stackedWidget->setCurrentIndex(1);

    // ui setup
    ui_->stopButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-stop"), QIcon(QLatin1String(":/resources/media-playback-stop.png"))));
    ui_->playPauseButton->setEnabled(true);
    ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-pause"), QIcon(QLatin1String(":/resources/media-playback-pause.png"))));
    ui_->playPauseButton->setToolTip(trUtf8("Pause"));

}

int AudioPlayer::currentIndex(const QString &file)
{
    int index = -1;
    for(int i = 0; i < Utility::audioSources.count(); i++) {
        if(Utility::audioSources.at(i).url().toString() == file) {
            index = i;
            break;
        }
    }
    return index;
}

void AudioPlayer::playNext()
{

}

void AudioPlayer::playPrevious()
{

}



#include "audioplayer.moc"
