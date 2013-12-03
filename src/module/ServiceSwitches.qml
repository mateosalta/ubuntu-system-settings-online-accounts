/*
 * Copyright (C) 2013 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
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

import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.OnlineAccounts 0.1

Column {
    id: root

    property variant account

    anchors.left: parent.left
    anchors.right: parent.right

    ListItem.Standard {
        text: i18n.dtr("ubuntu-system-settings-online-accounts", "Access to this account:")
    }

    AccountServiceModel {
        id: accountServices
        includeDisabled: true
        account: root.account.objectHandle
    }

    Repeater {
        model: accountServices
        delegate: ServiceItem {
            accountServiceHandle: model.accountServiceHandle
        }
    }
}
