#ifndef URLBUILDER_H
#define URLBUILDER_H

#include <QString>

namespace GrooveShark
{

class UrlBuilder
{
public:
    static QString getTokenUrl();
    static QString getSearchUrl();
    static QString getSongStreamKeyUrl();
    static QString getSongUrl();
    static QString getRefererUrl();

private:
    UrlBuilder() {}
    UrlBuilder( const UrlBuilder& ) {}
    static const QString s_server;
};
}

#endif // URLBUILDER_H
