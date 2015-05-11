#ifndef MAPBUILDER_H
#define MAPBUILDER_H

#include <QVariantMap>

namespace GroovesharkCom
{

class MapBuilder
{
public:
    static QVariantMap getTokenMap();
    static QVariantMap getSearchMap(const QString &query, const QString &token);
    static QVariantMap getDownloadMap(const QString &id, const QString &token);
    static QStringList jsqueue();

private:
    MapBuilder() {}
    MapBuilder( const MapBuilder& ) {}
    static const QByteArray s_secret;
    static const QByteArray s_uuid;

    static QVariantMap header();
};
}

#endif // MAPBUILDER_H

