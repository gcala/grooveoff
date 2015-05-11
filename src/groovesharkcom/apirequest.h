#ifndef APIREQUEST_H
#define APIREQUEST_H

#include "songlist.h"
#include "token.h"
#include "streamkey.h"
#include "downloader.h"

class QByteArray;
class QString;

namespace GroovesharkCom
{

class ApiRequestPrivate;

/*
 * This Class is the Frontend of libmygpo-qt.
 * Methods from this Class map the Web API of gpodder.net
 * and return the Results of the Requests.
 * Web API Documentation can be found here: http://wiki.gpodder.org/wiki/Web_Services/API_2
 */

class ApiRequest
{
public:
    static ApiRequest * instance();
    ~ApiRequest( );

    //SIMPLE API

    /*
     * Returns the Result for the Advanced API Call "Retrieving Top Tags"
     * @param count The number of Tags that should be returned - will be set to to 100 if > 100 or < 1
     * @return List of Tag Objects containing the Data from gPodder
     *
     */
    SongListPtr songs(QString match, QString token );

    TokenPtr token();

    StreamKeyPtr streamKey(uint id, QString token);

    DownloaderPtr downloadSong(QString path, QString fileName, uint id, QString token);

private:
    ApiRequest();

    ApiRequestPrivate* const d;

    static ApiRequest * s_instance;
};

}

#endif // APIREQUEST_H
