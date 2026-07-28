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


#include "Database.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace The {
    static Database* s_Database_instance = nullptr;

    Database* database()
    {
        if( !s_Database_instance )
            s_Database_instance = new Database();

        return s_Database_instance;
    }
}

namespace {

const int    c_schemaVersion  = 1;
const char   c_connectionName[] = "grooveoff";

/**
 * Every migration is a step from version n to version n+1. Once released a
 * migration is never edited again, only new ones are added: editing one means
 * that whoever already ran it ends up with a different schema from whoever
 * starts from scratch, and the symptom shows up months later on one machine.
 */
bool runStatements( QSqlDatabase &db, const QStringList &statements )
{
    for( const QString &statement : statements ) {
        QSqlQuery query( db );
        if( !query.exec( statement ) ) {
            qWarning() << "GrooveOff ::" << "database statement failed:"
                       << query.lastError().text() << "in" << statement;
            return false;
        }
    }

    return true;
}

} // namespace

Database::Database()
    : m_open( false )
{
    const QString &dataPath = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
    m_path = dataPath + QLatin1String( "/grooveoff.db" );

    if( !QDir().mkpath( dataPath ) ) {
        m_errorString = QStringLiteral( "cannot create the data directory %1" ).arg( dataPath );
        qWarning() << "GrooveOff ::" << m_errorString;
        return;
    }

    m_open = open();
}

Database::~Database()
{
    if( m_db.isOpen() )
        m_db.close();

    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase( QLatin1String( c_connectionName ) );
}

int Database::schemaVersion()
{
    return c_schemaVersion;
}

bool Database::isOpen() const
{
    return m_open;
}

QString Database::path() const
{
    return m_path;
}

QString Database::errorString() const
{
    return m_errorString;
}

QSqlDatabase Database::connection() const
{
    return m_db;
}

bool Database::open()
{
    if( !QSqlDatabase::isDriverAvailable( QLatin1String( "QSQLITE" ) ) ) {
        m_errorString = QStringLiteral( "the QSQLITE driver is not available" );
        qWarning() << "GrooveOff ::" << m_errorString;
        return false;
    }

    m_db = QSqlDatabase::addDatabase( QLatin1String( "QSQLITE" ), QLatin1String( c_connectionName ) );
    m_db.setDatabaseName( m_path );

    if( !m_db.open() ) {
        m_errorString = m_db.lastError().text();
        qWarning() << "GrooveOff ::" << "cannot open" << m_path << ":" << m_errorString;
        return false;
    }

    if( !checkIntegrity() ) {
        // The file is unusable: move it aside and start over rather than
        // leaving the user without an application.
        m_db.close();
        if( !moveAsideCorrupted() )
            return false;

        if( !m_db.open() ) {
            m_errorString = m_db.lastError().text();
            qWarning() << "GrooveOff ::" << "cannot reopen" << m_path << ":" << m_errorString;
            return false;
        }
    }

    const QStringList pragmas = {
        QStringLiteral( "PRAGMA journal_mode = WAL" ),
        QStringLiteral( "PRAGMA synchronous = NORMAL" ),
        QStringLiteral( "PRAGMA foreign_keys = ON" )
    };

    if( !runStatements( m_db, pragmas ) ) {
        m_errorString = QStringLiteral( "cannot set the pragmas" );
        return false;
    }

    if( !migrate() )
        return false;

    // The single row of app_state is an invariant of the schema, not a value:
    // everything that reads what was playing assumes it is there, and an UPDATE
    // against a table where it is not succeeds while changing nothing. That
    // failure has no symptom of its own — the application simply opens with an
    // empty track list — so the row is made sure of here rather than trusted to
    // the migration that created it once.
    QSqlQuery ensure( m_db );
    if( !ensure.exec( QStringLiteral( "INSERT OR IGNORE INTO app_state ( id, position_ms ) VALUES ( 1, 0 )" ) ) )
        qWarning() << "GrooveOff ::" << "cannot ensure the app_state row:" << ensure.lastError().text();

    // And the same for the playlist that is always there. It cannot be removed
    // through the interface, but a database can arrive here from anywhere — an
    // older backup, a hand edited file, a migration that was interrupted — and
    // a sidebar without it is a sidebar with nowhere to drop a track.
    QSqlQuery pinned( m_db );
    if( !pinned.exec( QStringLiteral(
            "INSERT INTO playlist ( name, kind, info_hash, pinned, created_at, updated_at )"
            " SELECT 'To listen', 1, NULL, 1, strftime( '%s', 'now' ), strftime( '%s', 'now' )"
            " WHERE NOT EXISTS ( SELECT 1 FROM playlist WHERE pinned = 1 )" ) ) ) {
        qWarning() << "GrooveOff ::" << "cannot ensure the pinned playlist:" << pinned.lastError().text();
    }

    return true;
}

bool Database::checkIntegrity()
{
    QSqlQuery query( m_db );

    if( !query.exec( QStringLiteral( "PRAGMA integrity_check" ) ) ) {
        qWarning() << "GrooveOff ::" << "integrity check failed to run:" << query.lastError().text();
        return false;
    }

    if( !query.next() )
        return false;

    const QString &result = query.value( 0 ).toString();
    if( result.compare( QLatin1String( "ok" ), Qt::CaseInsensitive ) != 0 ) {
        qWarning() << "GrooveOff ::" << "database integrity check says:" << result;
        return false;
    }

    return true;
}

bool Database::moveAsideCorrupted()
{
    const QString &stamp = QDateTime::currentDateTime().toString( QLatin1String( "yyyyMMdd-hhmmss" ) );
    const QString &target = m_path + QLatin1String( ".corrupted-" ) + stamp;

    if( !QFile::rename( m_path, target ) ) {
        m_errorString = QStringLiteral( "the database is corrupted and cannot be moved aside" );
        qWarning() << "GrooveOff ::" << m_errorString;
        return false;
    }

    // The write ahead log and its shared memory belong to the file we just
    // renamed; leaving them behind would confuse the fresh database.
    QFile::remove( m_path + QLatin1String( "-wal" ) );
    QFile::remove( m_path + QLatin1String( "-shm" ) );

    qWarning() << "GrooveOff ::" << "corrupted database moved to" << target;

    return true;
}

int Database::userVersion() const
{
    QSqlQuery query( m_db );

    if( !query.exec( QStringLiteral( "PRAGMA user_version" ) ) || !query.next() )
        return -1;

    return query.value( 0 ).toInt();
}

bool Database::setUserVersion( int version )
{
    QSqlQuery query( m_db );

    // Pragmas take no bound parameters, hence the assembled statement; version
    // is an int coming from this file only.
    return query.exec( QStringLiteral( "PRAGMA user_version = %1" ).arg( version ) );
}

bool Database::migrate()
{
    int version = userVersion();

    if( version < 0 ) {
        m_errorString = QStringLiteral( "cannot read the schema version" );
        qWarning() << "GrooveOff ::" << m_errorString;
        return false;
    }

    if( version > c_schemaVersion ) {
        // A newer build has been here. Refusing is safer than guessing what
        // its columns mean.
        m_errorString = QStringLiteral( "the database was written by a newer version of GrooveOff (schema %1, this build knows %2)" )
                            .arg( version ).arg( c_schemaVersion );
        qWarning() << "GrooveOff ::" << m_errorString;
        return false;
    }

    while( version < c_schemaVersion ) {
        const int target = version + 1;

        if( !m_db.transaction() ) {
            m_errorString = QStringLiteral( "cannot start the migration to schema %1" ).arg( target );
            qWarning() << "GrooveOff ::" << m_errorString;
            return false;
        }

        bool ok = false;
        switch( target ) {
            case 1:  ok = migrateToVersion1(); break;
            default: ok = false;               break;
        }

        if( !ok || !setUserVersion( target ) || !m_db.commit() ) {
            m_db.rollback();
            m_errorString = QStringLiteral( "migration to schema %1 failed" ).arg( target );
            qWarning() << "GrooveOff ::" << m_errorString;
            return false;
        }

        qDebug() << "GrooveOff ::" << "database schema migrated to version" << target;
        version = target;
    }

    return true;
}

bool Database::migrateToVersion1()
{
    const QStringList statements = {
        QStringLiteral(
            "CREATE TABLE torrent ("
            "    info_hash   TEXT    PRIMARY KEY,"
            "    magnet      TEXT    NOT NULL,"
            "    name        TEXT,"
            "    added_at    INTEGER NOT NULL,"
            "    last_access INTEGER NOT NULL,"
            "    bytes_done  INTEGER NOT NULL DEFAULT 0,"
            "    has_resume  INTEGER NOT NULL DEFAULT 0,"
            "    hidden      INTEGER NOT NULL DEFAULT 0"
            ")" ),
        QStringLiteral(
            "CREATE TABLE track ("
            "    id          INTEGER PRIMARY KEY,"
            "    info_hash   TEXT    NOT NULL REFERENCES torrent(info_hash) ON DELETE CASCADE,"
            "    file_index  INTEGER NOT NULL,"
            "    file_path   TEXT    NOT NULL,"
            "    file_size   INTEGER NOT NULL,"
            "    is_audio    INTEGER NOT NULL DEFAULT 0,"
            "    excluded    INTEGER NOT NULL DEFAULT 0,"
            "    title       TEXT,"
            "    artist      TEXT,"
            "    album       TEXT,"
            "    track_num   INTEGER,"
            "    year        INTEGER,"
            "    duration    INTEGER,"
            "    UNIQUE( info_hash, file_index )"
            ")" ),
        QStringLiteral(
            "CREATE TABLE playlist ("
            "    id          INTEGER PRIMARY KEY,"
            "    name        TEXT    NOT NULL,"
            "    kind        INTEGER NOT NULL,"
            "    info_hash   TEXT    REFERENCES torrent(info_hash) ON DELETE CASCADE,"
            // The one that is always there. A column and not a kind of its own:
            // it is a playlist made by hand in every respect the rest of the
            // program asks about, so the twenty-odd places that ask go on
            // giving the right answer without being touched.
            "    pinned      INTEGER NOT NULL DEFAULT 0,"
            "    created_at  INTEGER NOT NULL,"
            "    updated_at  INTEGER NOT NULL"
            ")" ),
        QStringLiteral(
            "CREATE TABLE playlist_track ("
            "    playlist_id INTEGER NOT NULL REFERENCES playlist(id) ON DELETE CASCADE,"
            "    position    INTEGER NOT NULL,"
            "    track_id    INTEGER NOT NULL REFERENCES track(id)    ON DELETE CASCADE,"
            "    PRIMARY KEY( playlist_id, position ),"
            // A track appears at most once in a list. The same track in three
            // lists is the point of having lists; the same track twice in one
            // is a list nobody meant to build, and the second copy has no
            // meaning the first does not already have.
            "    UNIQUE( playlist_id, track_id )"
            ")" ),
        QStringLiteral(
            "CREATE TABLE app_state ("
            "    id          INTEGER PRIMARY KEY CHECK( id = 1 ),"
            "    playlist_id INTEGER REFERENCES playlist(id) ON DELETE SET NULL,"
            "    track_id    INTEGER REFERENCES track(id)    ON DELETE SET NULL,"
            "    position_ms INTEGER NOT NULL DEFAULT 0"
            ")" ),
        QStringLiteral( "INSERT INTO app_state ( id, position_ms ) VALUES ( 1, 0 )" ),
        // Its name is stored but never shown: the interface says it in the
        // language it is running in, which a row in a database cannot do.
        QStringLiteral(
            "INSERT INTO playlist ( name, kind, info_hash, pinned, created_at, updated_at )"
            " VALUES ( 'To listen', 1, NULL, 1, strftime( '%s', 'now' ), strftime( '%s', 'now' ) )" ),
        QStringLiteral( "CREATE UNIQUE INDEX idx_playlist_torrent ON playlist( info_hash ) WHERE info_hash IS NOT NULL" ),
        QStringLiteral( "CREATE INDEX idx_playlist_track ON playlist_track( track_id )" ),
        QStringLiteral( "CREATE INDEX idx_track_hash     ON track( info_hash, excluded )" ),
        QStringLiteral( "CREATE INDEX idx_torrent_lru    ON torrent( last_access )" )
    };

    return runStatements( m_db, statements );
}

bool Database::transaction()
{
    return m_open && m_db.transaction();
}

bool Database::commit()
{
    return m_db.commit();
}

bool Database::rollback()
{
    return m_db.rollback();
}
