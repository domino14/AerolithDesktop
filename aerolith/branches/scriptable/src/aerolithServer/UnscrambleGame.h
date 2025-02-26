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

#ifndef _UNSCRAMBLE_GAME_H_
#define _UNSCRAMBLE_GAME_H_

#include <QtCore>
#include <QtSql>
#include <QObject>
#include "TableGame.h"
#include "databasehandler.h"
#include "SavedUnscrambleGame.h"
#include "commonDefs.h"
#define MAX_NUM_OPEN_QUESTIONS 1000000      // one million questions are allowed to be open, in total. (in unscramble game)

struct highScoreData
{
    QString userName;
    quint16 numCorrect;
    quint16 numSolutions;
    quint16 timeRemaining;
};

struct challengeInfo
{
    QHash <QString, highScoreData> *highScores;
    quint8 wordLength;
    QList <quint32> dbIndices;
};

struct alphagramInfo
{
    QString alphagram;
    QStringList solutions;
    alphagramInfo()
    {
    }
    alphagramInfo(QString a, QStringList s)
    {
        alphagram = a;
        solutions = s;
    }
};

class UnscrambleGame : public TableGame
{

    Q_OBJECT

public:

    QByteArray initialize();
    UnscrambleGame(Table*);
    ~UnscrambleGame();

    void gameStartRequest(ClientSocket*);
//    void guessSent(ClientSocket*, QString);
    void handleMiscPacket(ClientSocket*, quint8);
    void gameEndRequest(ClientSocket*);
    void playerJoined(ClientSocket*);
    void playerLeftGame(ClientSocket*);

    void correctAnswerSent(ClientSocket*, quint8 space, quint8 specificAnswer);

    void cleanupBeforeDelete();
    void endGame();
    void startGame();

    static void generateDailyChallenges();
    static void loadWordLists();
    static void sendLists(ClientSocket*);
    static void sendListSpaceUsage(ClientSocket* socket);
    static QHash <QString, challengeInfo> challenges;

    static bool midnightSwitchoverToggle;

    static int numTotalQuestions;
    int thisMaxQuestions;

    bool savingAllowed;
    void savedListExists(bool exists);
    void gotQuizArray(QList<quint32> quizArray, QList<quint32> missedArray, QByteArray sugArray);
private:

    SavedUnscrambleGame sug;

    bool autosaveOnQuit;

    static QByteArray wordListDataToSend;

    void prepareTableQuestions();
 //   void sendUserCurrentAlphagrams(ClientSocket*);
    void sendUserCurrentQuestions(ClientSocket*);
    QString lexiconName;



    struct UnscrambleGameQuestionData
    {
        QSet <quint8> notYetSolved; // which of these question's anagrams have not yet been solved?
        quint32 probIndex;  // this question's probability index
        quint8 numNotYetSolved;
        bool exists;
    };

    //  QHash <QString, playerData> playerDataHash;
    QSqlQuery query;
    QString wordList;
    QString wordListOriginal;
    bool wroteToMissedFileThisRound;
    bool gameStarted;
    bool countingDown;
    bool startEnabled;
    QTimer gameTimer;
    QTimer countdownTimer;
    quint8 numRacksThisRound;
    quint16 currentTimerVal;
    quint16 tableTimerVal;
    quint8 countdownTimerVal;
    quint8 listType;
    quint16 totalNumberQuestions;
    quint8 cycleState;
    quint16 numTotalSolutions;
    quint16 numTotalSolvedSoFar;

//    QHash <QString, QString> gameSolutions;
//    QHash <QString, quint8> alphagramIndices;

    QList <UnscrambleGameQuestionData> unscrambleGameQuestions;

    QList <quint32> missedArray;
    QList <quint32> quizArray;


    quint16 quizIndex;
    QVector <alphagramInfo> *alphaInfo;

    quint8 wordLength; // the word length of this list , not used for "named list" type (see commonDefs.h for list types)
    quint32 lowProbIndex, highProbIndex;    // not used for "named list" type

    void setQuizArrayRange(quint32 low, quint32 high);
    bool thisTableSwitchoverToggle;

    quint16 numTotalRacks;
    quint16 numRacksSeen;

    void sendCorrectAnswerPacket(quint8, quint8, quint8);

    void sendTimerValuePacket(quint16);
    void sendGiveUpPacket(QString);

    void sendNumQuestionsPacket();
    void sendListCompletelyExhaustedMessage();
    void sendListExhaustedMessage();
    void performSpecificSitActions(ClientSocket*);
    void performSpecificStandActions(ClientSocket*);

    void sendSavingAllowed(ClientSocket* client);
    quint8 userlistMode;

    void saveProgress(ClientSocket* client);

    void requestListExists(QString, QString, QString);
    bool pendingListExistsRequest;
    void generateQuizArray();
    void requestGenerateQuizArray();
    bool pendingQuizArray;
    bool pendingPrepareNextGame;
private slots:
    void updateGameTimer();
    void updateCountdownTimer();

};

#endif
