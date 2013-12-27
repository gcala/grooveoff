/*
 * GrooveOff - Offline Grooveshark.com music
 * Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "mpris_manager.h"
#include "dbus/mpris2.h"

#include <QDebug>

MprisManager::MprisManager(QObject *parent) : QObject(parent)
{
    m_mpris2 = 0;

    reloadSettings();
}


MprisManager::~MprisManager()
{
    if(m_mpris2 != 0)
    {
        delete m_mpris2;
        m_mpris2 = 0;
    }
}

void MprisManager::reloadSettings()
{
//     if(SETTINGS()->_useMpris)
    if(true)
    {
      if(!m_mpris2)
        m_mpris2 =  new Mpris2(this);
    }
    else
    {
      if(m_mpris2 != 0) {
        delete m_mpris2;
        m_mpris2 = 0;
      }
    }
}



