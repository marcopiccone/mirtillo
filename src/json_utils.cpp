#include "json_utils.h"

#include <QFile>
#include <QJsonDocument>

bool loadJsonObject(const QString& path, QJsonObject& out)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return false;

    out = doc.object();
    return true;
}

QString readFileType(const QJsonObject& content)
{
    const QJsonObject extra = content.value("extraMetaData").toObject();
    QString t = extra.value("fileType").toString().trimmed();  // "pdf"|"epub"|"notebook"
    if (!t.isEmpty())
        return t;

    t = content.value("fileType").toString().trimmed();
    return t; // può essere vuoto: verrà gestito a valle
}

QJsonArray readPageTags(const QJsonObject& content)
{
    const QJsonObject extra = content.value("extraMetaData").toObject();
    QJsonArray a = extra.value("pageTags").toArray();
    if (!a.isEmpty())
        return a;

    return content.value("pageTags").toArray();
}

QHash<QString,int> buildPageMap(const QJsonObject& content)
{
    QHash<QString,int> map;

    auto parsePagesArray = [&](const QJsonArray& pages) {
        for (const auto& v : pages) {
            const QJsonObject p = v.toObject();
            const QString pid = p.value("id").toString().trimmed();
            int pageNo = -1;
            const QJsonObject redir = p.value("redir").toObject();
            if (!redir.isEmpty())
                pageNo = redir.value("value").toInt(-1);

            if (!pid.isEmpty() && pageNo >= 0)
                map.insert(pid, pageNo);
        }
    };

    // 1) cPages.pages
    const QJsonObject cPages = content.value("cPages").toObject();
    QJsonArray pages = cPages.value("pages").toArray();
    if (!pages.isEmpty())
        parsePagesArray(pages);

    // 2) fallback: root-level "pages"
    if (map.isEmpty()) {
        pages = content.value("pages").toArray();
        if (!pages.isEmpty())
            parsePagesArray(pages);
    }

    return map;
}