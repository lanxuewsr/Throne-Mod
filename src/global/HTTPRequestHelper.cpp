#include "include/global/HTTPRequestHelper.hpp"

#include <QNetworkProxy>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QFile>
#include <QApplication>
#include <QMap>
#include <QStringList>



#include "include/global/Configs.hpp"
#include "include/database/ProfilesRepo.h"
#include "include/ui/mainwindow.h"
#include "include/global/DeviceDetailsHelper.hpp"

namespace Configs_network {
    namespace {
        QString normalizedAppRequestProxyMode() {
            auto mode = Configs::dataManager->settingsRepo->app_request_proxy_mode.trimmed();
            if (mode.isEmpty()) {
                mode = Configs::dataManager->settingsRepo->net_use_proxy
                    ? QString::fromLatin1(AppRequestProxyMode::Auto)
                    : QString::fromLatin1(AppRequestProxyMode::Direct);
            }
            return mode;
        }

        QString appRequestProxyHost() {
            return Configs::dataManager->settingsRepo->inbound_address == "::"
                ? QStringLiteral("127.0.0.1")
                : Configs::dataManager->settingsRepo->inbound_address;
        }

        void applyProfileProxyAuth(AppRequestProxyConfig& config, const std::shared_ptr<Configs::Profile>& profile) {
            if (profile != nullptr && profile->local_auth_enabled) {
                config.username = profile->local_auth_user;
                config.password = profile->local_auth_pass;
            }
        }

        void applySystemProxyAuth(AppRequestProxyConfig& config) {
            if (Configs::dataManager->settingsRepo->inbound_auth) {
                config.username = Configs::dataManager->settingsRepo->inbound_user;
                config.password = Configs::dataManager->settingsRepo->inbound_pass;
            }
        }
    }

    AppRequestProxyConfig NetworkRequestHelper::ResolveAppRequestProxy(bool forceProxy) {
        AppRequestProxyConfig config;
        const auto mode = normalizedAppRequestProxyMode();
        if (mode == AppRequestProxyMode::Direct) return config;
        const bool proxyRequested = forceProxy || mode != AppRequestProxyMode::Direct;
        if (!proxyRequested) return config;

        if (Configs::dataManager->settingsRepo->started_id < 0 &&
            !Configs::dataManager->settingsRepo->started_port_bound_mode) {
            config.error = QObject::tr("Request with proxy but no profile started.");
            return config;
        }

        config.host = appRequestProxyHost();

        auto useSystemProxyNode = [&]() {
            const int id = Configs::dataManager->settingsRepo->system_proxy_profile_id;
            if (id < 0) {
                config.error = QObject::tr("App request proxy is set to System Proxy Node, but no System Proxy node is selected.");
                return;
            }
            if (!Configs::dataManager->settingsRepo->spmode_system_proxy) {
                config.error = QObject::tr("App request proxy is set to System Proxy Node, but System Proxy mode is not enabled.");
                return;
            }
            config.enabled = true;
            config.port = Configs::dataManager->settingsRepo->inbound_socks_port;
            applySystemProxyAuth(config);
        };

        auto useSelectedLocalPortNode = [&]() {
            const int id = Configs::dataManager->settingsRepo->app_request_proxy_profile_id;
            auto profile = Configs::dataManager->profilesRepo->GetProfile(id);
            if (profile == nullptr || profile->local_port <= 0) {
                config.error = QObject::tr("App request proxy selected node is missing or has no local port binding.");
                return;
            }
            if (!Configs::dataManager->settingsRepo->started_port_bound_mode ||
                !Configs::dataManager->settingsRepo->started_port_bound_ids.contains(id)) {
                config.error = QObject::tr("App request proxy selected local port node is not running.");
                return;
            }
            config.enabled = true;
            config.port = profile->local_port;
            applyProfileProxyAuth(config, profile);
        };

        auto useSingleRunningLocalPortNode = [&]() -> bool {
            if (!Configs::dataManager->settingsRepo->started_port_bound_mode) return false;

            QList<std::shared_ptr<Configs::Profile>> candidates;
            for (const auto id : Configs::dataManager->settingsRepo->started_port_bound_ids) {
                auto profile = Configs::dataManager->profilesRepo->GetProfile(id);
                if (profile != nullptr && profile->local_port > 0) candidates << profile;
            }
            if (candidates.size() != 1) return false;

            config.enabled = true;
            config.port = candidates.first()->local_port;
            applyProfileProxyAuth(config, candidates.first());
            return true;
        };

        if (mode == AppRequestProxyMode::SystemProxyNode) {
            useSystemProxyNode();
        } else if (mode == AppRequestProxyMode::SelectedLocalPortNode) {
            useSelectedLocalPortNode();
        } else {
            if (Configs::dataManager->settingsRepo->spmode_system_proxy) {
                useSystemProxyNode();
            } else if (!useSingleRunningLocalPortNode()) {
                if (Configs::dataManager->settingsRepo->started_port_bound_mode) {
                    config.error = QObject::tr("App request proxy Auto mode found multiple or no running local port bindings. Please select a local port node explicitly.");
                } else {
                    config.enabled = true;
                    config.port = Configs::dataManager->settingsRepo->inbound_socks_port;
                    applySystemProxyAuth(config);
                }
            }
        }

        return config;
    }

    HTTPResponse NetworkRequestHelper::HttpGet(const QString &url, bool sendHwid, bool useProxy) {
        QNetworkRequest request;
        QNetworkAccessManager accessManager;
        accessManager.setTransferTimeout(10000);
        request.setUrl(url);
        auto proxyConfig = ResolveAppRequestProxy(useProxy);
        if (!proxyConfig.error.isEmpty()) return HTTPResponse{proxyConfig.error};
        if (proxyConfig.enabled) {
            QNetworkProxy p;
            p.setType(QNetworkProxy::HttpProxy);
            p.setHostName(proxyConfig.host);
            p.setPort(proxyConfig.port);
            if (!proxyConfig.username.isEmpty() || !proxyConfig.password.isEmpty()) {
                p.setUser(proxyConfig.username);
                p.setPassword(proxyConfig.password);
            }
            accessManager.setProxy(p);
        }
        // Set attribute
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, Configs::dataManager->settingsRepo->GetUserAgent());
        if (Configs::dataManager->settingsRepo->net_insecure) {
            QSslConfiguration c;
            c.setPeerVerifyMode(QSslSocket::PeerVerifyMode::VerifyNone);
            request.setSslConfiguration(c);
        }
        //Attach HWID and device info headers if enabled in settings
        if (sendHwid) {
            auto details = GetDeviceDetails();

            // Parse custom parameters if provided
            QMap<QString, QString> customParams;
            if (!Configs::dataManager->settingsRepo->sub_custom_hwid_params.isEmpty()) {
                QStringList pairs = Configs::dataManager->settingsRepo->sub_custom_hwid_params.split(',');
                for (const QString &pair : pairs) {
                    QString trimmed = pair.trimmed();
                    int eqPos = trimmed.indexOf('=');
                    if (eqPos > 0) {
                        QString key = trimmed.left(eqPos).trimmed();
                        QString value = trimmed.mid(eqPos + 1).trimmed();
                        // Validate: key must be one of the allowed parameters, value must not contain newlines
                        if (!key.isEmpty() && !value.isEmpty() &&
                            !value.contains('\n') && !value.contains('\r') &&
                            value.length() < 1000) { // Reasonable length limit
                            QString lowerKey = key.toLower();
                            // Only accept known parameter keys
                            if (lowerKey == "hwid" || lowerKey == "os" ||
                                lowerKey == "osversion" || lowerKey == "model") {
                                customParams[lowerKey] = value;
                            }
                        }
                    }
                }
            }

            // Use custom values if provided, otherwise use default values
            QString hwid = customParams.contains("hwid") ? customParams["hwid"] : details.hwid;
            QString os = customParams.contains("os") ? customParams["os"] : details.os;
            QString osVersion = customParams.contains("osversion") ? customParams["osversion"] : details.osVersion;
            QString model = customParams.contains("model") ? customParams["model"] : details.model;

            if (!hwid.isEmpty()) request.setRawHeader("x-hwid", hwid.toUtf8());
            if (!os.isEmpty()) request.setRawHeader("x-device-os", os.toUtf8());
            if (!osVersion.isEmpty()) request.setRawHeader("x-ver-os", osVersion.toUtf8());
            if (!model.isEmpty()) request.setRawHeader("x-device-model", model.toUtf8());
        }
        //
        auto _reply = accessManager.get(request);
        connect(_reply, &QNetworkReply::sslErrors, _reply, [](const QList<QSslError> &errors) {
            QStringList error_str;
            for (const auto &err: errors) {
                error_str << err.errorString();
            }
            MW_show_log(QString("SSL Errors: %1 %2").arg(error_str.join(","), Configs::dataManager->settingsRepo->net_insecure ? "(Ignored)" : ""));
        });
        // Wait for response
        QEventLoop loop;
        connect(_reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        //
        auto result = HTTPResponse{_reply->error() == QNetworkReply::NetworkError::NoError ? "" : _reply->errorString(),
                                       _reply->readAll(), _reply->rawHeaderPairs()};
        _reply->deleteLater();
        return result;
    }

    QString NetworkRequestHelper::GetHeader(const QList<QPair<QByteArray, QByteArray>> &header, const QString &name) {
        for (const auto &p: header) {
            if (QString(p.first).toLower() == name.toLower()) return p.second;
        }
        return "";
    }

    QString NetworkRequestHelper::DownloadAsset(const QString &url, const QString &fileName) {
        QNetworkRequest request;
        QNetworkAccessManager accessManager;
        request.setUrl(url);
        auto proxyConfig = ResolveAppRequestProxy();
        if (!proxyConfig.error.isEmpty()) return proxyConfig.error;
        if (proxyConfig.enabled) {
            QNetworkProxy p;
            p.setType(QNetworkProxy::HttpProxy);
            p.setHostName(proxyConfig.host);
            p.setPort(proxyConfig.port);
            if (!proxyConfig.username.isEmpty() || !proxyConfig.password.isEmpty()) {
                p.setUser(proxyConfig.username);
                p.setPassword(proxyConfig.password);
            }
            accessManager.setProxy(p);
        }
        if (Configs::dataManager->settingsRepo->net_insecure) {
            QSslConfiguration c;
            c.setPeerVerifyMode(QSslSocket::PeerVerifyMode::VerifyNone);
            request.setSslConfiguration(c);
        }

        auto _reply = accessManager.get(request);
        connect(_reply, &QNetworkReply::sslErrors, _reply, [](const QList<QSslError> &errors) {
            QStringList error_str;
            for (const auto &err: errors) {
                error_str << err.errorString();
            }
            MW_show_log(QString("SSL Errors: %1 %2").arg(error_str.join(","), Configs::dataManager->settingsRepo->net_insecure ? "(Ignored)" : ""));
        });
        connect(_reply, &QNetworkReply::downloadProgress, _reply, [&](qint64 bytesReceived, qint64 bytesTotal)
        {
            runOnUiThread([=]{
                GetMainWindow()->setDownloadReport(DownloadProgressReport{fileName, bytesReceived, bytesTotal}, true);
                GetMainWindow()->UpdateDataView();
            });
        });
        QEventLoop loop;
        connect(_reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        runOnUiThread([=]
        {
            GetMainWindow()->setDownloadReport({}, false);
            GetMainWindow()->UpdateDataView(true);
        });
        _reply->deleteLater();
        if(_reply->error() != QNetworkReply::NetworkError::NoError) {
            return _reply->errorString();
        }

        auto filePath = Configs::GetBasePath()+ "/" + fileName;
        auto file = QFile(filePath);
        if (file.exists()) {
            file.remove();
        }
        if (!file.open(QIODevice::WriteOnly)) {
            return QObject::tr("Could not open file.");
        }
        file.write(_reply->readAll());
        file.close();
        return "";
    }

} // namespace Configs_network
