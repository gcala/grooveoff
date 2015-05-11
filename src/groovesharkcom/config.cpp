#include "config_p.h"

#include <QString>
#include <QStringBuilder>
#include <QLatin1String>

using namespace GroovesharkCom;

Config* Config::s_instance = 0;

ConfigPrivate::ConfigPrivate( Config* qq ) : q( qq ), m_gsBaseUrl( QUrl( QLatin1String( "https://grooveshark.com" ) ) ), m_userAgentPrefix( QString() )
{

}

ConfigPrivate::~ConfigPrivate()
{

}

QUrl ConfigPrivate::gsBaseUrl() const
{
    return m_gsBaseUrl;
}

void ConfigPrivate::setGsBaseUrl( const QUrl& gsBaseUrl )
{
    this->m_gsBaseUrl = gsBaseUrl;
}

QByteArray ConfigPrivate::userAgent() const
{
    QString userAgent;
    if ( !m_userAgentPrefix.isEmpty() )
        userAgent = m_userAgentPrefix % QLatin1String( " " );
    userAgent = userAgent % QLatin1String( "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/536.5 (KHTML, like Gecko) Chrome/19.0.1084.56 Safari/536.5 " );
    return userAgent.toLatin1();
}

QLatin1String ConfigPrivate::host() const
{
    return QLatin1String("grooveshark.com");
}

QString ConfigPrivate::userAgentPrefix() const
{
    return m_userAgentPrefix;
}

void ConfigPrivate::setUserAgentPrefix( const QString& prefix )
{
    m_userAgentPrefix = prefix;
}

QString ConfigPrivate::contentType() const
{
    QString contentType;
    contentType = contentType % QLatin1String( "application/json" );
    return contentType;
}

Config::Config() : d( new ConfigPrivate( this ) )
{

}

Config::~Config()
{
    delete d;
}

QUrl Config::gsBaseUrl() const
{
    return d->gsBaseUrl();
}

void Config::setGsBaseUrl(const QUrl& gsBaseUrl)
{
    d->setGsBaseUrl( gsBaseUrl );
}

QByteArray Config::userAgent() const
{
    return d->userAgent();
}

QLatin1String Config::host() const
{
    return d->host();
}

QString Config::userAgentPrefix() const
{
    return d->userAgentPrefix();
}

void Config::setUserAgentPrefix(const QString& prefix)
{
    d->setUserAgentPrefix( prefix );
}

QString Config::contentType() const
{
    return d->contentType();
}

Config* Config::instance()
{
    if ( !s_instance ) {
        s_instance = new Config;
    }

    return s_instance;
}

