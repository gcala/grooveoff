/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2014  Giuseppe Calà <jiveaxe@gmail.com>

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


#include "App.h"

#include <kdsingleapplication.h>

int main( int argc, char** argv )
{
    App app( argc, argv );

    KDSingleApplication singleInstance;

    // a second instance asks the running one to show up, then gets out of the way
    if( !singleInstance.isPrimaryInstance() ) {
        singleInstance.sendMessage( QByteArrayLiteral( "raise" ) );
        return 0;
    }

    QObject::connect( &singleInstance, &KDSingleApplication::messageReceived,
                      &app, [&app]( const QByteArray & ) { app.raiseWindow(); } );

    app.start();

    return app.exec();
}
