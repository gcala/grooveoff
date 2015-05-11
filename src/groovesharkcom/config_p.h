#ifndef CONFIG_P_H
#define CONFIG_P_H

#include "config.h"

namespace GroovesharkCom
{

class ConfigPrivate
{

public:
     ConfigPrivate( Config* qq );
    ~ConfigPrivate();

    QUrl gsBaseUrl() const;
    void setGsBaseUrl( const QUrl& gsBaseUrl );

    QByteArray userAgent() const;
    QLatin1String host() const;

    QString userAgentPrefix() const;
    void setUserAgentPrefix( const QString& prefix );

    QString contentType() const;
private:
    Config* q;
    QUrl m_gsBaseUrl;
    QString m_userAgentPrefix;

};

}

#endif // CONFIG_P_H
