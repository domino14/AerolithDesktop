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

#include "UnscrambleGame.h"
#include "commonDefs.h"
#include "databasehandler.h"
extern QByteArray block;
extern QDataStream out;

const quint8 COUNTDOWN_TIMER_VAL = 3;
const quint8 maxRacks = 50;
QHash <QString, challengeInfo> UnscrambleGame::challenges;
bool UnscrambleGame::midnightSwitchoverToggle;
int UnscrambleGame::numTotalQuestions = 0;

QByteArray UnscrambleGame::wordListDataToSend;
//QVector<QVector <QVector<alphagramInfo> > > UnscrambleGame::alphagramData;


UnscrambleGame::UnscrambleGame(Table* table) : TableGame(table)
{
    qDebug() << "UnscrambleGame constructor";
}

QByteArray UnscrambleGame::initialize(DatabaseHandler* dbHandler)
{
    this->dbHandler = dbHandler;
    autosaveOnQuit = false;
    wroteToMissedFileThisRound = false;

    quint8 tableTimerValMin;

    table->host->connData.in >> listType;   // TODO need to check to make sure client can't send malicious packet
    // that lacks some of these fields!

    bool onePlayerTable = false;

    if (table->maxPlayers == 1) onePlayerTable = true;

    savingAllowed = false;
    if (listType == LIST_TYPE_NAMED_LIST)
    {
        table->host->connData.in >> wordList;
        if (onePlayerTable) savingAllowed = true;
    }
    else if (listType == LIST_TYPE_INDEX_RANGE_BY_WORD_LENGTH)
    {
        table->host->connData.in >> wordLength >> lowProbIndex >> highProbIndex;
        wordList = QString("The %1s: %2 to %3").
                   arg(wordLength).arg(lowProbIndex).arg(highProbIndex);
        if (onePlayerTable) savingAllowed = true;
    }
    else if (listType == LIST_TYPE_ALL_WORD_LENGTH)
    {
        table->host->connData.in >> wordLength;
        wordList = QString("The %1s").arg(wordLength);
        if (onePlayerTable) savingAllowed = true;
    }
    else if (listType == LIST_TYPE_DAILY_CHALLENGE)
    {
        table->host->connData.in >> wordList;
        // don't allow saving here. (why? don't know yet. anyone want it?)
    }
    else if (listType == LIST_TYPE_USER_LIST)
    {
        table->host->connData.in >> userlistMode;
        table->host->connData.in >> wordListOriginal;

        wordList = wordListOriginal;
        autosaveOnQuit = true;
        if (onePlayerTable) savingAllowed = true;
    }

    table->host->connData.in >> lexiconName >> cycleState >> tableTimerValMin;

    if (dbHandler->availableDatabases.contains(lexiconName))
    {
        wordDbConName = lexiconName + "DB_server";  // this is how it is in databaseHandler. todo perhaps should make this a variable.

    }


    connect(&gameTimer, SIGNAL(timeout()), this, SLOT(updateGameTimer()));
    connect(&countdownTimer, SIGNAL(timeout()), this, SLOT(updateCountdownTimer()));

    gameStarted = false;
    countingDown = false;
    startEnabled = true;
    tableTimerVal = (quint16)tableTimerValMin * (quint16)60;
    currentTimerVal = tableTimerVal;
    countdownTimerVal = COUNTDOWN_TIMER_VAL;

    numRacksSeen = 0;
    numTotalRacks = 0;

    generateQuizArray();

    if (cycleState == TABLE_TYPE_DAILY_CHALLENGES)
    {
        if (wordLength <= 3) tableTimerVal = 100;
        if (wordLength == 4) tableTimerVal = 150;
        if (wordLength == 5) tableTimerVal = 220;
        if (wordLength == 6) tableTimerVal = 240;
        if (wordLength >= 7 && wordLength <= 8) tableTimerVal = 270;
        if (wordLength > 8) tableTimerVal = 300;
    }



    // compute array to be sent out as table information array
    writeHeaderData();
    out << (quint8) SERVER_NEW_TABLE;
    out << (quint16) table->tableNumber;
    out << (quint8) GAME_TYPE_UNSCRAMBLE;
    out << lexiconName;
    out << wordList;
    out << table->maxPlayers;
    out << table->isPrivate;
    fixHeaderLength();

    return block;



}


UnscrambleGame::~UnscrambleGame()
{
    qDebug() << "UnscrambleGame destructor";

    //	gameTimer->deleteLater();
    //	countdownTimer->deleteLater();

}

void UnscrambleGame::playerJoined(ClientSocket* client)
{
    if (gameStarted)
        sendUserCurrentQuestions(client);

    sendSavingAllowed(client);
    // possibly send scores, solved solutions, etc. in the future.
    //    if (table->peopleInTable.size() == 1 && table->maxPlayers == 1 && cycleState != TABLE_TYPE_DAILY_CHALLENGES)
    //    {
    //        qDebug() << "playerJoined";
    //
    //
    //    }

        if (table->originalHost == client && savingAllowed)
        {
            if (autosaveOnQuit)
                table->sendTableMessage("Autosave on quit is now on. If you are disconnected, your game "
                                        "will be automatically saved.");
            else
            {


                if (dbHandler->savedListExists(lexiconName, wordList, client->connData.userName))
                {
                    table->sendTableMessage("You already have a saved list named: " + wordList + ". If you "
                                            "click Save, you will overwrite this list! If you do not wish "
                                            "to do this, please select your saved list from 'My Lists' when "
                                            "creating a new table.");


                }
                else
                    table->sendTableMessage("Autosave on quit is off. To turn it on, you must click the Save button"
                                            " to save your current progress. If you are disconnected, the game will "
                                            "then be automatically saved.");

            }

        }

}

void UnscrambleGame::cleanupBeforeDelete()
{
    numTotalQuestions -= thisMaxQuestions;
}

void UnscrambleGame::playerLeftGame(ClientSocket* socket)
{
    // if this is the ONLY player, stop game.
    if (table->peopleInTable.size() == 1 && table->maxPlayers == 1)
    {
        gameEndRequest(socket);
    }

    if (socket == table->originalHost && autosaveOnQuit && savingAllowed)
    {
        table->sendTableMessage("Auto-saving progress...");
        // probably no one will see this unless there was someone in the table.
        saveProgress(socket);

    }

    //    if (socket == table->originalHost)
    //    {
    //        if (listType == LIST_TYPE_USER_LIST)
    //        {
    //            /* the host is no longer able to send out indices to the table.  inform the players of this */
    //            table->sendTableMessage("The original host of the table has left. "
    //                                    "Since the host was the one to create this list, you cannot quiz on it any longer.");
    //
    //
    //        }
    //    }
}

void UnscrambleGame::gameStartRequest(ClientSocket* client)
{
    if (!client->connData.isSitting) return;    // can't request to start the game if you're standing
    if (startEnabled == true && gameStarted == false && countingDown == false)
    {
        bool startTheGame = true;

        sendReadyBeginPacket(client->connData.seatNumber);
        client->playerData.readyToPlay = true;

        foreach (ClientSocket* thisClient, table->sittingList)
        {
            if (thisClient && thisClient->playerData.readyToPlay == false)
            {
                startTheGame = false;
                break;
            }
        }


        if (startTheGame == true)
        {
            countingDown = true;
            countdownTimerVal = 3;
            countdownTimer.start(1000);
            sendTimerValuePacket(countdownTimerVal);
            if (cycleState == TABLE_TYPE_DAILY_CHALLENGES)
            {
                if (challenges.contains(wordList))
                {
                    if (table->originalHost)
                    {
                        if (challenges.value(wordList).
                            highScores->contains(table->originalHost->connData.userName.toLower()))
                            table->sendTableMessage(table->originalHost->connData.userName + " has already played this "
                                                    "challenge. You can play again, "
                                                    "but only the first game's results count toward today's high scores!");
                    }
                }
            }

        }
    }

}


void UnscrambleGame::correctAnswerSent(ClientSocket* socket, quint8 space, quint8 specificAnswer)
{
    if (socket->connData.isSitting)
    {
        if (space < numRacksThisRound)
        {
            //         qDebug() << "  " << unscrambleGameQuestions[space].numNotYetSolved << unscrambleGameQuestions[space].notYetSolved;
            if (unscrambleGameQuestions[space].notYetSolved.contains(specificAnswer))
            {
                unscrambleGameQuestions[space].notYetSolved.remove(specificAnswer);
                unscrambleGameQuestions[space].numNotYetSolved--;
                numTotalSolvedSoFar++;

                sendCorrectAnswerPacket(socket->connData.seatNumber, space, specificAnswer);

                if (savingAllowed && unscrambleGameQuestions[space].numNotYetSolved == 0)
                {
                    // completely solved one entry
                    if (sug.brandNew)
                    {
                        sug.brandNew = false;
                        sug.curQuizSet = sug.origIndices;
                    }
                    qDebug() << "Answered correctly";

                    //       Q_ASSERT(currentSug.curQuizList.contains(wordQuestions.at(space).probIndex));
                    qDebug() << sug.curQuizSet.contains(unscrambleGameQuestions[space].probIndex);

                    sug.curQuizSet.remove(unscrambleGameQuestions[space].probIndex);
                }
                if (numTotalSolvedSoFar == numTotalSolutions) endGame();
            }
        }
    }
}

void UnscrambleGame::gameEndRequest(ClientSocket* socket)
{
    if (gameStarted == true)
    {
        bool giveUp = true;

        sendGiveUpPacket(socket->connData.userName);
        socket->playerData.gaveUp = true;

        foreach (ClientSocket* thisSocket, table->sittingList)
        {
            if (thisSocket && thisSocket->playerData.gaveUp == false)
            {
                giveUp = false;
                break;
            }
        }
        if (giveUp == true)
        {
            currentTimerVal = 0;
            endGame();
        }
    }

}

void UnscrambleGame::startGame()
{

    countingDown = false;
    countdownTimer.stop();
    // code for starting the game
    // steps:
    //1. list should already be loaded when table was created
    //2. send to everyone @ the table:
    //    - maxRacks alphagrams
    gameStarted = true;
    wroteToMissedFileThisRound = false;
    prepareTableQuestions();
    sendGameStartPacket();

    foreach (ClientSocket* socket, table->peopleInTable)
        sendUserCurrentQuestions(socket);


    sendTimerValuePacket(tableTimerVal);
    sendNumQuestionsPacket();
    currentTimerVal = tableTimerVal;
    gameTimer.start(1000);
    thisTableSwitchoverToggle = midnightSwitchoverToggle;

    if (savingAllowed && table->sittingList.at(0) != table->originalHost)
    {
        savingAllowed = false;
        table->sendTableMessage("The quiz taker was not the original creator of this table. You are not "
                                "allowed to save progress anymore.");
    }
}

void UnscrambleGame::performSpecificSitActions(ClientSocket* socket)
{
    socket->playerData.gaveUp = false;
    socket->playerData.readyToPlay = false;
    socket->playerData.score = 0;
}

void UnscrambleGame::performSpecificStandActions(ClientSocket* socket)
{
    socket->playerData.gaveUp = false;
    socket->playerData.readyToPlay = false;
    socket->playerData.score = 0;

    if (savingAllowed && socket == table->originalHost)
    {
        table->sendTableMessage("If someone takes your seat and starts the quiz, saving won't be allowed "
                                "anymore. You will lose your progress if you intend to save this game.");
    }
}

void UnscrambleGame::endGame()
{
    gameTimer.stop();
    gameStarted = false;

    sendGameEndPacket();
    sendTimerValuePacket((quint16)0);

    foreach (ClientSocket* client, table->peopleInTable)
    {
        client->playerData.readyToPlay = false;
        client->playerData.gaveUp = false;
    }
    // if in cycle mode, update list
    QList <quint32> missedThisRound;
    if (cycleState == TABLE_TYPE_CYCLE_MODE)
    {
        if (!wroteToMissedFileThisRound)
        {
            wroteToMissedFileThisRound = true;
            for (int i = 0; i < numRacksThisRound; i++)
            {
                if (unscrambleGameQuestions.at(i).numNotYetSolved > 0)
                {
                    missedThisRound << unscrambleGameQuestions.at(i).probIndex;


                }

            }
        }
        qDebug() << "Missed to date:" << missedArray.size() << "racks.";




    }
    else if (cycleState == TABLE_TYPE_DAILY_CHALLENGES) // daily challenges
    {
        startEnabled = false;
        table->sendTableMessage("This challenge is over! To see scores or to try another challenge, exit the table "
                                "and make the appropriate selections with the Challenges button.");

        // search for player.
        if (table->originalHost && challenges.contains(wordList))
        {
            if (challenges.value(wordList).highScores->contains(table->originalHost->connData.userName.toLower()))
                table->sendTableMessage(table->originalHost->connData.userName + " has already played this challenge. "
                                        "These results will not count towards this day's high scores.");
            else
            {
                if (midnightSwitchoverToggle == thisTableSwitchoverToggle)
                {
                    highScoreData tmp;
                    tmp.userName = table->originalHost->connData.userName;
                    tmp.numSolutions = numTotalSolutions;
                    tmp.numCorrect = numTotalSolvedSoFar;
                    tmp.timeRemaining = currentTimerVal;
                    challenges.value(wordList).highScores->insert(table->originalHost->connData.userName.toLower(), tmp);

                }
                else
                    table->sendTableMessage("The daily lists have changed while you were playing. Please try again with the new list!");
            }

        }

    }
#if QT_VERSION >= 0x040500
    missedArray.append(missedThisRound);
#else
    foreach (quint32 p, missedThisRound)
      missedArray.append(p);
#endif
    if (savingAllowed)
    {
        if (sug.brandNew)
        {
            sug.brandNew = false;
            sug.curQuizSet = sug.origIndices;
        }
        foreach (quint32 index, missedThisRound)
        {
            sug.curQuizSet.remove(index);
            sug.curMissedSet.insert(index);
            if (!sug.seenWholeList)
            {
                // also put it here if we haven't seen the whole list.
                sug.firstMissed.insert(index);
            }
        }
    }

}


void UnscrambleGame::updateCountdownTimer()
{
    countdownTimerVal--;
    sendTimerValuePacket(countdownTimerVal);
    if (countdownTimerVal == 0)
        startGame();
}

void UnscrambleGame::updateGameTimer()
{
    currentTimerVal--;
    sendTimerValuePacket(currentTimerVal);
    if (currentTimerVal == 0)
        endGame();
}

void UnscrambleGame::setQuizArrayRange(quint32 low, quint32 high)
{
    QList <quint32> indexList;
    numTotalRacks = high-low+1;
    getUniqueRandomNumbers(indexList, low, high, numTotalRacks);


    for (quint32 i = low; i <= high; i++)
        quizArray << indexList.at(i-low);
}

void UnscrambleGame::generateQuizArray()
{

    QTime timer;
    timer.start();

    switch (listType)
    {
    case LIST_TYPE_NAMED_LIST:
        {
            QSqlQuery query(QSqlDatabase::database(wordDbConName));
            query.setForwardOnly(true);

            query.prepare("SELECT probindices from wordlists where listname = ?");
            query.bindValue(0, wordList);
            query.exec();

            QByteArray indices;

            while (query.next())
            {
                indices = query.value(0).toByteArray();
            }

            //            QDataStream stream(indices);
            QDataStream stream(indices);
            quint8 type, length;
            stream >> type >> length;   /*TODO remove length from the parameters */

            if (type == 1)
            {
                // a list of indices
                quint32 size;
                stream >> size;
                QList <quint32> tempList;
                QList <quint32> indexList;
                for (int i = 0; i < size; i++)
                    indexList << 0;   // dummy, to resize to 'size'

                quint32 index;
                getUniqueRandomNumbers(tempList, 0, size-1, size);
                for (quint32 i = 0; i < size; i++)
                {
                    stream >> index;
                    indexList[tempList.at(i)] = index;
                }

                for (quint32 i = 0; i < size; i++)
                    quizArray << indexList.at(i);

                numTotalRacks = size;
            }
            else
                qCritical() << "ERROR: type != 1 !";    // we always have a list of indices for named lists

            sug.initialize(quizArray);
            // and set SUG accordingly
            break;
        }


    case LIST_TYPE_INDEX_RANGE_BY_WORD_LENGTH:
        {
            lowProbIndex += (wordLength << 24);
            highProbIndex += (wordLength << 24);    // this is the encoding we use for probability.


            setQuizArrayRange(lowProbIndex, highProbIndex); //numTotalRacks is also set in this function
            // and set SUG accordingly
            sug.initializeWithIndexRange(lowProbIndex, highProbIndex);

            break;
        }
    case  LIST_TYPE_ALL_WORD_LENGTH:
        {
            lowProbIndex = 1 + (wordLength << 24);  // probability 1 is the first
            if (wordLength >=2 && wordLength <= 15)
                highProbIndex = DatabaseHandler::lexiconMap.value(lexiconName).alphagramsPerLength[wordLength]
                                + (wordLength << 24);

            setQuizArrayRange(lowProbIndex, highProbIndex); //numTotalRacks also set in this function

            sug.initializeWithIndexRange(lowProbIndex, highProbIndex);
            // and set SUG accordingly

            break;
        }
    case LIST_TYPE_USER_LIST:
        {
            QString pathName = dbHandler->getSavedListAbsolutePath(lexiconName, wordListOriginal,
                                                                table->originalHost->connData.userName);

            QFile f(pathName);
            f.open(QIODevice::ReadOnly);
            QByteArray ba = f.readAll();

            sug.populateFromByteArray(ba);
            QList <quint32> qindices;
            QList <quint32> mindices;
            switch (userlistMode)
            {

            case UNSCRAMBLEGAME_USERLIST_MODE_RESTART:
                sug.initialize(sug.origIndices);

                qindices = sug.origIndices.toList();
                mindices.clear();

                break;

            case UNSCRAMBLEGAME_USERLIST_MODE_FIRSTMISSED:
                qindices = sug.firstMissed.toList();
                mindices.clear();

                sug.curQuizSet = sug.firstMissed;
                sug.curMissedSet.clear();
                break;

            case UNSCRAMBLEGAME_USERLIST_MODE_CONTINUE:
                if (sug.brandNew)
                {
                    qindices = sug.origIndices.toList();
                    mindices.clear();
                }
                else
                {
                    qindices = sug.curQuizSet.toList();
                    mindices = sug.curMissedSet.toList();
                }
             //   qDebug() << "Index:" << qindices.at(0);
                break;
            }

            // a list of indices
            quint32 size = qindices.size();

            // TODO modularize this; put into a function. shouldn't repeat code.
            if (size > 0)
            {
                QList <quint32> tempList;
                QList <quint32> indexList;
                for (int i = 0; i < size; i++)
                    indexList << 0;   // dummy, to resize to 'size'

                quint32 index;
                getUniqueRandomNumbers(tempList, 0, size-1, size);
                for (quint32 i = 0; i < size; i++)
                {
                    indexList[tempList.at(i)] = qindices.at(i);
                }

                for (quint32 i = 0; i < size; i++)
                    quizArray << indexList.at(i);
            }
            numTotalRacks = size;

            size = mindices.size();
            if (size > 0)
            {
                QList <quint32> tempList;
                QList <quint32> indexList;
                for (int i = 0; i < size; i++)
                    indexList << 0;   // dummy, to resize to 'size'

                quint32 index;
                getUniqueRandomNumbers(tempList, 0, size-1, size);
                for (quint32 i = 0; i < size; i++)
                {
                    indexList[tempList.at(i)] = mindices.at(i);
                }

                for (quint32 i = 0; i < size; i++)
                    missedArray << indexList.at(i);

            }

            break;
        }
    case LIST_TYPE_DAILY_CHALLENGE:
        {
            // daily challenges
            // just copy the daily challenge array

            if (challenges.contains(wordList))
            {
                wordLength = challenges.value(wordList).wordLength;
                quizArray = challenges.value(wordList).dbIndices;
            }

            numTotalRacks = maxRacks;   // this line may be unnecessary, numTotalRacks is set in prepareTableAlphagrams before the start of the game


            break;
        }

    }


    quizIndex = 0;

    neverStarted = true;
    thisMaxQuestions = missedArray.size() + quizArray.size();
    numTotalQuestions += thisMaxQuestions;

}

void UnscrambleGame::prepareTableQuestions()
{
    unscrambleGameQuestions.clear();

    numTotalSolutions = 0;
    numTotalSolvedSoFar = 0;
    if (cycleState == TABLE_TYPE_CYCLE_MODE && quizIndex == quizArray.size())
    {
        quizArray = missedArray;		// copy missedArray to quizArray
        quizIndex = 0;	// and set the index back at the beginning
        missedArray.clear();	// also clear the missed array, cuz we're starting a new one.
        table->sendTableMessage("The list has been exhausted. Now quizzing on missed list.");
        sendListExhaustedMessage();
        numRacksSeen = 0;
        numTotalRacks = quizArray.size();

        if (savingAllowed)
        {
            /* we are finally done with the main quiz */
            if (!sug.seenWholeList)
            {
                sug.seenWholeList = true;
            }

            sug.curQuizSet = sug.curMissedSet;
            sug.curMissedSet.clear();
        }



    }

    QTime timer;
    timer.start();

    QSqlQuery query(QSqlDatabase::database(wordDbConName));

    query.exec("BEGIN TRANSACTION");
    query.prepare(QString("SELECT words from alphagrams where probability = ?"));
    numRacksThisRound = 0;

    for (quint8 i = 0; i < maxRacks; i++)
    {
        UnscrambleGameQuestionData thisQuestionData;

        if (quizIndex == quizArray.size())
        {
            if (i == 0)
            {
                if (cycleState != TABLE_TYPE_DAILY_CHALLENGES)
                {
                    table->sendTableMessage("This list has been completely exhausted. Please exit table and have a nice day.");
                    sendListCompletelyExhaustedMessage();
                }
                else
                {
                    table->sendTableMessage("This challenge is over. To view scores, please exit table and "
                                            "select 'Get today's scores' from the 'Challenges' button.");
                    sendListCompletelyExhaustedMessage();
                }
                if (savingAllowed)
                {
                    if (sug.curMissedSet.size() != 0 || sug.curQuizSet.size() != 0)
                    {
                        qCritical() << "ERROR in SUG";
                    }
                }
            }
            break;
        }
        else
        {
            numRacksSeen++;
            numRacksThisRound++;
            quint32 index = quizArray.at(quizIndex);

            query.bindValue(0, index);
            query.exec();

            while (query.next())
            {
                QStringList sols = query.value(0).toString().split(" ");
                int size = sols.size();
                thisQuestionData.numNotYetSolved = size;
                thisQuestionData.probIndex = index;
                for (int k = 0; k < size; k++)
                {
                    thisQuestionData.notYetSolved.insert(k);
                    numTotalSolutions++;
                }
            }
            quizIndex++;
            thisQuestionData.exists = true;
        }
        unscrambleGameQuestions.append(thisQuestionData);
    }
    query.exec("END TRANSACTION");
    qDebug() << "finished PrepareTableQuestions, time=" << timer.elapsed();

}

void UnscrambleGame::sendUserCurrentQuestions(ClientSocket* socket)
{
    writeHeaderData();
    out << (quint8) SERVER_TABLE_COMMAND;
    out << table->tableNumber;
    out << (quint8) SERVER_TABLE_QUESTIONS;
    out << (quint8) numRacksThisRound;
    for (int i = 0; i < numRacksThisRound; i++)
    {
        out << unscrambleGameQuestions.at(i).probIndex;
        out << unscrambleGameQuestions.at(i).numNotYetSolved;
        foreach (quint8 notSolved, unscrambleGameQuestions.at(i).notYetSolved)
            out << notSolved;
    }
    fixHeaderLength();
    socket->write(block);
}

void UnscrambleGame::sendSavingAllowed(ClientSocket* socket)
{
    writeHeaderData();
    out << (quint8) SERVER_TABLE_COMMAND;
    out << table->tableNumber;
    out << (quint8) SERVER_TABLE_UNSCRAMBLEGAME_SAVING_ALLOWED;
    out << savingAllowed;
    fixHeaderLength();
    socket->write(block);
}

void UnscrambleGame::sendListExhaustedMessage()
{
    writeHeaderData();
    out << (quint8) SERVER_TABLE_COMMAND;
    out << table->tableNumber;
    out << (quint8) SERVER_TABLE_UNSCRAMBLEGAME_MAIN_QUIZ_DONE;
    fixHeaderLength();
    table->sendGenericPacket();
}

void UnscrambleGame::sendListCompletelyExhaustedMessage()
{
    writeHeaderData();
    out << (quint8) SERVER_TABLE_COMMAND;
    out << table->tableNumber;
    out << (quint8) SERVER_TABLE_UNSCRAMBLEGAME_FULL_QUIZ_DONE;
    fixHeaderLength();
    table->sendGenericPacket();
}

void UnscrambleGame::sendTimerValuePacket(quint16 timerValue)
{
    writeHeaderData();
    out << (quint8) SERVER_TABLE_COMMAND;
    out << (quint16) table->tableNumber;
    out << (quint8)SERVER_TABLE_TIMER_VALUE;
    out << timerValue;
    fixHeaderLength();
    table->sendGenericPacket();
}

void UnscrambleGame::sendNumQuestionsPacket()
{
    writeHeaderData();
    out << (quint8) SERVER_TABLE_COMMAND;
    out << (quint16) table->tableNumber;
    out << (quint8) SERVER_TABLE_NUM_QUESTIONS;
    out << numRacksSeen;
    out << numTotalRacks;
    fixHeaderLength();
    table->sendGenericPacket();

}

void UnscrambleGame::sendGiveUpPacket(QString username)
{
    writeHeaderData();
    out << (quint8) SERVER_TABLE_COMMAND;
    out << (quint16) table->tableNumber;
    out << (quint8)SERVER_TABLE_GIVEUP;
    out << username;
    fixHeaderLength();
    table->sendGenericPacket();
}



void UnscrambleGame::sendCorrectAnswerPacket(quint8 seatNumber, quint8 space, quint8 specificAnswer)
{
    writeHeaderData();
    out << (quint8) SERVER_TABLE_COMMAND;
    out << (quint16) table->tableNumber;
    out << (quint8) SERVER_TABLE_CORRECT_ANSWER;
    out << seatNumber << space << specificAnswer;
    fixHeaderLength();
    table->sendGenericPacket();
}

void UnscrambleGame::handleMiscPacket(ClientSocket* socket, quint8 header)
{
    switch (header)
    {
    case CLIENT_TABLE_UNSCRAMBLEGAME_CORRECT_ANSWER:
        {
            quint8 space, specificAnswer;
            socket->connData.in >> space >> specificAnswer;
            correctAnswerSent(socket, space, specificAnswer);

        }
        break;
    case CLIENT_TABLE_UNSCRAMBLEGAME_SAVE_REQUEST:
        {
            if (savingAllowed && socket == table->originalHost)
            {
                // perform save
                saveProgress(socket);
                if (autosaveOnQuit == false)
                {
                    autosaveOnQuit = true;
                    table->sendTableMessage("Autosave on quit is now on.");
                }
            }
        }
        break;
    default:
        socket->disconnectFromHost();


    }


}

void UnscrambleGame::saveProgress(ClientSocket* socket)
{
    bool result =
            dbHandler->saveGame(sug, lexiconName, wordList, socket->connData.userName);
    if (result)
        table->sendTableMessage("Saved " + table->originalHost->connData.userName + "'s game!");
    else
    {
        table->sendTableMessage("Was unable to save game! You may be over quota!");
        qDebug() << "Unable to save game!" << lexiconName << wordList << socket->connData.userName;
        return;
    }


    // and send information about saved game back to client.
    QStringList sl = dbHandler->getSingleListLabels(lexiconName, socket->connData.userName,
                                                    wordList);

    writeHeaderData();
    out << (quint8) SERVER_UNSCRAMBLEGAME_LISTDATA_CLEARONE;
    out << lexiconName;
    out << wordList;
    fixHeaderLength();
    socket->write(block);

    writeHeaderData();
    out << (quint8) SERVER_UNSCRAMBLEGAME_LISTDATA_ADDONE;
    out << lexiconName;
    out << sl;
    fixHeaderLength();
    socket->write(block);

    writeHeaderData();
    out << (quint8) SERVER_UNSCRAMBLEGAME_LISTDATA_DONE;
    fixHeaderLength();
    socket->write(block);       // to get it to resize columns

    writeListSpaceUsage(socket, dbHandler);
}

/*************** static and utility functions ****************/



void UnscrambleGame::loadWordLists(DatabaseHandler* dbHandler)
{


    writeHeaderData();
    out << (quint8) SERVER_WORD_LISTS;		// word lists
    out << (quint8) dbHandler->availableDatabases.size();
    foreach(QString lexiconName, dbHandler->availableDatabases)
        out << lexiconName.toAscii();
    out << (quint8) 2;           // two types right now, regular, and challenge.
    out << (quint8) 'R';	// regular

    for (quint8 i = 0; i < dbHandler->availableDatabases.size(); i++)
    {
        QSqlQuery query(DatabaseHandler::lexiconMap.value(dbHandler->availableDatabases[i]).db_server);
        query.exec("SELECT listname from wordlists");
        QStringList wordLists;
        while (query.next())
            wordLists << query.value(0).toString();

        out << i;   // lexicon index
        out << (quint16)wordLists.size();
        foreach (QString wordlistName, wordLists)
            out << wordlistName.toAscii();
    }

    out << (quint8) 'D';
    for (quint8 i = 0; i < dbHandler->availableDatabases.size(); i++)
    {
        out << i;
        if (dbHandler->availableDatabases.at(i) != "Volost")
        {
            out << (quint16)14;
            for (int j = 2; j <= 15; j++)
                out << QString("Today's " + dbHandler->availableDatabases.at(i) + " %1s").arg(j).toAscii();
        }
        else
        {
            out << (quint16)2;
            out << QString("Today's Volost 6s").toAscii();
            out << QString("Today's Volost 7s").toAscii();
        }
    }
    fixHeaderLength();
    wordListDataToSend = block; // copy to a block so that we don't have to recompute this everytime someone logs in
}

void UnscrambleGame::sendLists(ClientSocket* socket)
{
    socket->write(wordListDataToSend);
}



void UnscrambleGame::generateDailyChallenges(DatabaseHandler* dbHandler)
{
    /* test randomness of algorithm */

    /*
    QVector <int> numFreq;
    int low = 0, high = 499;
    int poolSize = high-low+1;
    int subsetSize = 50;
    numFreq.resize(poolSize);
    numFreq.fill(0, poolSize);

    for (int i = 0; i < 1000000; i++)
    {
        QVector <quint16> nums;
        getUniqueRandomNumbers(nums, low, high, subsetSize);
        for (int j = 0; j < subsetSize; j++)
            numFreq[nums[j]]++;
    }
    qDebug() << numFreq;
    //
    return;
    */
    midnightSwitchoverToggle = !midnightSwitchoverToggle;

    QList <challengeInfo> vals = challenges.values();
    foreach (challengeInfo ci, vals)
        delete ci.highScores;

    challenges.clear();
    QTime timer;

    for (quint8 i = 0; i < dbHandler->availableDatabases.size(); i++)
    {
        QSqlQuery query(DatabaseHandler::lexiconMap.value(dbHandler->availableDatabases[i]).db_server);
        query.exec("BEGIN TRANSACTION");
        for (int j = 2; j <= 15; j++)
        {
            query.exec(QString("SELECT count(*) from alphagrams where length = %1").arg(j));
            int wordCount = 0;
            while (query.next())
            {
                wordCount = query.value(0).toInt();
            }
            if (wordCount != 0)
            {
                challengeInfo tmpChallenge;
                tmpChallenge.highScores = new QHash <QString, highScoreData>;
                getUniqueRandomNumbers(tmpChallenge.dbIndices, 1 + (j << 24), wordCount+(j << 24), qMin(wordCount, 50));
                tmpChallenge.wordLength = j;
                QString challengeName = QString("Today's " + dbHandler->availableDatabases.at(i) + " %1s").arg(j);
                challenges.insert(challengeName, tmpChallenge);
                qDebug() << "Generated" << challengeName << tmpChallenge.dbIndices;
            }

        }
        query.exec("END TRANSACTION");
    }
    qDebug() << "Generated daily challenges!";
}


void UnscrambleGame::writeListSpaceUsage(ClientSocket* socket, DatabaseHandler* dbHandler)
{
    quint32 usage, max;
    dbHandler->getListSpaceUsage(socket->connData.userName, usage, max);

    writeHeaderData();
    out << (quint8) SERVER_UNSCRAMBLEGAME_LISTSPACEUSAGE;
    out << usage << max;
    fixHeaderLength();
    socket->write(block);

}
