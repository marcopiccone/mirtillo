#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QString>

// Carica un file JSON come QJsonObject
bool loadJsonObject(const QString& path, QJsonObject& out);

// Legge il tipo di file con fallback: extraMetaData.fileType → root.fileType → ""
QString readFileType(const QJsonObject& content);

// Restituisce i pageTags: extraMetaData.pageTags → root.pageTags
QJsonArray readPageTags(const QJsonObject& content);

// Costruisce una mappa pageId → pageNumber da cPages.pages[].redir.value (con fallback root.pages)
QHash<QString,int> buildPageMap(const QJsonObject& content);