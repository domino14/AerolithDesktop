//    Copyright 2007, 2008, 2009, Cesar Del Solar  <delsolar@gmail.com>
//    This file is part of Aerolith.
//
//    Aerolith is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Aerolith is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Aerolith.  If not, see <http://www.gnu.org/licenses/>.

#ifndef _TABLE_H_
#define _TABLE_H_

#include <QtCore>
#include "ClientSocket.h"
#include "commonDefs.h"

class TableGame;


class Table : public QObject
{
    Q_OBJECT
public:

    enum tablePacketHeaders
    {
        GAME_STARTED, GAME_ENDED, CHAT_SENT, GUESS_RIGHT, TIMER_VALUE, READY_TO_BEGIN, GAVE_UP
            };

    Table(QObject* parent = 0);
    ~Table();
    QByteArray initialize(ClientSocket* tableCreator, quint16 tableNumber);
    quint16 tableNumber;
    quint64 tableID; // a unique ID for this table. as long as no one makes 18 quintillion tables this should
                    // be fine until the next reboot.

    QString tableName;
    quint8 maxPlayers;

    QList <ClientSocket*> peopleInTable;
    QVector <ClientSocket*> sittingList;
    QSet <ClientSocket*> inviteList;
    bool canJoin(ClientSocket*);
    bool isPrivate;

    TableGame* tableGame;
    ClientSocket *host;
    ClientSocket *originalHost;

    void personJoined(ClientSocket*);

    bool setTablePrivacy(ClientSocket*, bool p);
    void processAvatarID(ClientSocket* socket, quint8 id);
    void sendChatSentPacket(QString, QString);
    void sendGenericPacket();
    void sendTableMessage(QString);
    void sendHostChangePacket(ClientSocket* host);
    quint8 gameType;
    QByteArray tableInformationArray;
    void removePersonFromTable(ClientSocket*);

    void sendSuccessfulSitPacket(QString, quint8);
    void sendSuccessfulStandPacket(QString, quint8);

    void trySitting(ClientSocket*, quint8);
    void tryStanding(ClientSocket*);
    void cleanupBeforeDelete();
private:
    void sendAvatarChangePacket(ClientSocket*);

signals:
    void databaseRequest(QByteArray);
};

#endif
