#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QUrl>

namespace GroovesharkCom
{

class ConfigPrivate;

class Config
{
public:
    static Config* instance();

    QUrl gsBaseUrl() const;
    void setGsBaseUrl( const QUrl& gsBaseUrl );

    QByteArray userAgent() const;
    QLatin1String host() const;

    QString userAgentPrefix() const;
    void setUserAgentPrefix( const QString& prefix );

    QString contentType() const;

private:
    Config();
    ~Config();

    static Config* s_instance;

    ConfigPrivate* const d;
};
}

#endif // CONFIG_H
