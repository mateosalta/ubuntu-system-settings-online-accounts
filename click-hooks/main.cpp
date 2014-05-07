/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This file is part of online-accounts-ui
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Accounts/Manager>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

static QString stripVersion(const QString &appId)
{
    QStringList components = appId.split('_');
    if (components.count() != 3) return appId;

    /* Click packages have a profile of the form
     *  $name_$application_$version
     * (see https://wiki.ubuntu.com/SecurityTeam/Specifications/ApplicationConfinement/Manifest#Click)
     *
     * So we just need to strip out the last part.
     */
    components.removeLast();
    return components.join('_');
}

class LibAccountsFile: public QDomDocument {
public:
    LibAccountsFile(const QFileInfo &hookFileInfo);

    void checkId(const QString &shortAppId);
    void addProfile(const QString &appId);
    bool writeTo(const QString &fileName) const;

private:
    QFileInfo m_hookFileInfo;
};

LibAccountsFile::LibAccountsFile(const QFileInfo &hookFileInfo):
    QDomDocument(),
    m_hookFileInfo(hookFileInfo)
{
    QFile file(hookFileInfo.filePath());
    if (file.open(QIODevice::ReadOnly)) {
        setContent(&file);
        file.close();
    }
}

void LibAccountsFile::checkId(const QString &shortAppId)
{
    /* checks that the root element's "id" attributes is consistent with the
     * file name */
    QDomElement root = documentElement();
    root.setAttribute(QStringLiteral("id"), shortAppId);
}

void LibAccountsFile::addProfile(const QString &appId)
{
    QDomElement root = documentElement();
    QDomElement elem = createElement(QStringLiteral("profile"));
    elem.appendChild(createTextNode(appId));
    root.appendChild(elem);
}

bool LibAccountsFile::writeTo(const QString &fileName) const
{
    /* Make sure that the target directory exists */
    QDir fileAsDirectory(fileName);
    fileAsDirectory.mkpath("..");

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    bool ok = (file.write(toByteArray(2)) > 0);
    file.close();

    if (ok) {
        return true;
    } else {
        QFile::remove(fileName);
        return false;
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    /* Go through the hook files in ~/.local/share/online-accounts-hooks/ and
     * check if they have already been processed into a file under
     * ~/.local/share/accounts/{providers,services,service-types,applications}/;
     * if not, open the hook file, write the APP_ID somewhere in it, and
     * save the result in the location where libaccounts expects to find it.
     */

    QStringList fileTypes;
    fileTypes << QStringLiteral("provider") <<
        QStringLiteral("service") <<
        QStringLiteral("service-type") <<
        QStringLiteral("application");

    // This is ~/.local/share/
    const QString localShare =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir hooksDirIn(localShare + "/" HOOK_FILES_SUBDIR);
    Q_FOREACH(const QFileInfo &fileInfo, hooksDirIn.entryInfoList()) {
        const QString fileType = fileInfo.suffix();
        // Filter out the files which we don't support
        if (!fileTypes.contains(fileType)) continue;

        // Our click hook sets the base name to the APP_ID
        QString appId = fileInfo.completeBaseName();

        /* When publishing this file for libaccounts, we want to strip
         * the version number out. */
        QString shortAppId = stripVersion(appId);

        /* When building the destination file name, use the file suffix with an
         * "s" appended: remember that libaccounts uses
         * ~/.local/share/accounts/{providers,services,service-types,applications}.
         * */
        QString destination = QString("%1/accounts/%2s/%3.%2").
            arg(localShare).arg(fileInfo.suffix()).arg(shortAppId);

        /* If the destination is there, we assume it's up to date */
        if (QFile::exists(destination)) continue;

        LibAccountsFile xml = LibAccountsFile(fileInfo);
        if (xml.isNull()) continue;

        xml.checkId(shortAppId);
        xml.addProfile(appId);
        xml.writeTo(destination);
    }

    /* To ensure that all the installed services are parsed into
     * libaccounts' DB, we enumerate them now.
     */
    Accounts::Manager *manager = new Accounts::Manager;
    manager->serviceList();
    delete manager;

    return EXIT_SUCCESS;
}

