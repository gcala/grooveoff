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


#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QString>

class Database;

namespace The {
    Database* database();
}

/**
  * @class Database
  * @short The sqlite connection, its pragmas and its schema migrations.
  *
  * Holds the catalogue of torrents and tracks, the playlists and the cache
  * index. Everything under the data directory is precious: what can be thrown
  * away without losing anything lives in the cache directory instead, and never
  * in here.
  *
  * No QSql type is meant to leave src/db. Catalog and PlaylistStore are the
  * only classes that touch the connection.
  */
class Database
{
public:
    friend Database* The::database();

    ~Database();

    /** current schema version this build knows how to produce */
    static int schemaVersion();

    /** true when the connection is open and the schema is up to date */
    bool isOpen() const;

    /** the file the database lives in */
    QString path() const;

    /** empty unless opening went wrong */
    QString errorString() const;

    /**
     * The connection, for Catalog and PlaylistStore. Invalid when isOpen()
     * is false, in which case every query fails and the application runs
     * without persistence rather than refusing to start.
     */
    QSqlDatabase connection() const;

    bool transaction();
    bool commit();
    bool rollback();

private:
    Database();
    Q_DISABLE_COPY( Database )

    bool open();

    /**
     * Runs PRAGMA integrity_check. A corrupt file is renamed aside and a fresh
     * one is created: losing the catalogue is bad, refusing to launch is worse.
     */
    bool checkIntegrity();
    bool moveAsideCorrupted();

    /** applies every missing migration, each one in its own transaction */
    bool migrate();
    bool migrateToVersion1();

    int  userVersion() const;
    bool setUserVersion( int version );

    QSqlDatabase m_db;
    QString      m_path;
    QString      m_errorString;
    bool         m_open;
};

#endif // DATABASE_H
