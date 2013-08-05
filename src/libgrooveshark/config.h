#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QUrl>

namespace GrooveShark
{

class ConfigPrivate;

class Config
{
public:
    static Config* instance();

    QUrl gsBaseUrl() const;
    void setGsBaseUrl( const QUrl& gsBaseUrl );

    QString userAgent() const;

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
