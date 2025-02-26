#ifndef _TABLE_H_
#define _TABLE_H_

#include <QtCore>
//#include "TableGame.h"
//#include "UnscrambleGame.h"
#include "ClientSocket.h"


class TableGame;


class tableData
{
 public:
  enum gameModes
    { GAMEMODE_UNSCRAMBLE, GAMEMODE_ANAGRAMS, GAMEMODE_TRIVIA };

 enum tablePacketHeaders
   {
     GAME_STARTED, GAME_ENDED, CHAT_SENT, GUESS_RIGHT, TIMER_VALUE, READY_TO_BEGIN, GAVE_UP
   };
 
  ~tableData();
  void initialize(quint16 tableNumber, QString tableName, quint8 maxPlayers, 
		  ClientSocket* tableCreator, quint8 cycleState, quint8 tableTimer, gameModes gameMode, QString additionalDescriptor);
  quint16 tableNumber;
  //QString wordListDescriptor; // todo: change this to table name
  QString tableName;
  QList <ClientSocket*> peopleList;
  QList<ClientSocket*> sittingList;
  quint8 maxPlayers;
  bool canJoin; 
  TableGame* tableGame;
  ClientSocket* tableHost;
  
  bool tryToSit(ClientSocket*, int);
  void standUp(ClientSocket*, int);

  void sendChatSentPacket(QString, QString);
  void sendGenericPacket();
  void sendTableMessage(QString);
  // anagrams mode?
  gameModes gameMode; 
  QByteArray letters; // the letters that are showing on the board

};

#endif
