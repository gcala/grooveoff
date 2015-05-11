#include "urlbuilder.h"
#include "config.h"

#include <QString>
#include <QStringBuilder>
#include <QLatin1String>

using namespace GroovesharkCom;

const QString UrlBuilder::s_server = Config::instance()->gsBaseUrl().toString();

QString UrlBuilder::getTokenUrl()
{
    return s_server % QLatin1String( "/more.php" );
}

QString UrlBuilder::getSearchUrl()
{
    return s_server % QLatin1String( "/more.php?" );
}

QString UrlBuilder::getSongStreamKeyUrl()
{
    return s_server % QLatin1String( "/more.php?" );
}

QString UrlBuilder::getSongUrl()
{
    return s_server % QLatin1String( "/more.php?" );
}

QString UrlBuilder::getRefererUrl()
{
    return s_server % QLatin1String( "JSQueue.swf?" );
}

