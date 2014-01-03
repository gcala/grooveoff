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


#include "Utility.h"
#include "PlaylistItem.h"

#include <QFontMetrics>
#include <QDesktopServices>
#include <QDir>

const int Utility::coverSize  = 40; // possible values are 40,50,70,80,90,120
const int Utility::buttonSize = 24;
const int Utility::marginSize = 5;
QString Utility::coversCachePath = "";
QString Utility::downloadPath = "";
QString Utility::token = "";
//QList<Phonon::MediaSource> Utility::audioSources;
//QList< PlaylistItemPtr > Utility::playlist;

QString Utility::elidedText(const QString& text,
                            const Qt::TextElideMode& elideMode,
                            const int& width,
                            const QFont::Weight &weight)
{
    QFont systemFont;
    systemFont.setWeight(weight);
    QFontMetrics fm(systemFont);

    return fm.elidedText(text, elideMode, width);
}

QFont Utility::font(const QFont::Weight& weight, const int &delta)
{
    QFont systemFont;
    systemFont.setPointSize(systemFont.pointSize() + delta);
    systemFont.setWeight(weight);

    return systemFont;
}

QFont Utility::monoFont(const QFont::Weight& weight, const int &delta)
{
    QFont systemFont;
    systemFont.setStyleHint(QFont::Monospace);
    systemFont.setPointSize(systemFont.pointSize() + delta);
    systemFont.setWeight(weight);

    return systemFont;
}

QString Utility::fileName(const GrooveShark::SongPtr &song)
{
    return song->songName().replace('/','-') + " - " + song->artistName().replace('/','-');
}
