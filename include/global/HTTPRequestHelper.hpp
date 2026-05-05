#pragma once

#include <QObject>
#include <functional>

namespace Configs_network {
    namespace AppRequestProxyMode {
        constexpr const char* Direct = "direct";
        constexpr const char* Auto = "auto";
        constexpr const char* SystemProxyNode = "system_proxy_node";
        constexpr const char* SelectedLocalPortNode = "selected_local_port_node";
    }

    struct AppRequestProxyConfig {
        bool enabled = false;
        QString error;
        QString host;
        int port = 0;
        QString username;
        QString password;
    };

    struct HTTPResponse {
        QString error;
        QByteArray data;
        QList<QPair<QByteArray, QByteArray>> header;
    };

    struct DownloadProgressReport
    {
        QString fileName;
        qint64 downloadedSize;
        qint64 totalSize;
    };

    class NetworkRequestHelper : QObject {
        Q_OBJECT

        explicit NetworkRequestHelper(QObject *parent) : QObject(parent){};

        ~NetworkRequestHelper() override = default;
        ;

    public:
        static AppRequestProxyConfig ResolveAppRequestProxy(bool forceProxy = false);

        static HTTPResponse HttpGet(const QString &url, bool sendHwid = false, bool useProxy = false);

        static QString GetHeader(const QList<QPair<QByteArray, QByteArray>> &header, const QString &name);

        static QString DownloadAsset(const QString &url, const QString &fileName);
    };
} // namespace Configs_network

using namespace Configs_network;
