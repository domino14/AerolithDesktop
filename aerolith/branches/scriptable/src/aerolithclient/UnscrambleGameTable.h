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

#ifndef _UNSCRAMBLE_GAME_TABLE_H_
#define _UNSCRAMBLE_GAME_TABLE_H_

#include <QtCore>
#include <QtGui>
#include <QtSql>
#include "ui_tableForm.h"

#include "ui_solutionsForm.h"
#include "ui_tableCustomizationForm.h"
#include "tile.h"
#include "chip.h"
#include "wordRectangle.h"
#include "GameTable.h"
#include "commonDefs.h"


class UnscrambleGameTable : public GameTable
{
    Q_OBJECT

public:
    UnscrambleGameTable(QWidget* parent, Qt::WindowFlags f);
    ~UnscrambleGameTable();
    void resetTable(quint16, QString, QString);
    void leaveTable();
    void addPlayer(QString);
    void removePlayer(QString);
    void addPlayers(QStringList);

    void setPrivacy(bool p);
    void setupForGameStart();
    void gotChat(QString);
    void gotTimerValue(quint16 timerval);
    void gotWordListInfo(QString);
    void mainQuizDone();
    void fullQuizDone();

    void clearSolutionsDialog();
    void markSolutionsTable();

    void addNewWord(int, quint32, QString, QStringList, quint8, QSet <quint8>);
    void clearAllWordTiles();
    void answeredCorrectly(quint8 seat, quint8 space, quint8 specificAnswer);

    void setReadyIndicator(quint8 seat);
    void clearReadyIndicators();
//    void setCurrentSug(SavedUnscrambleGame sug); // TODO delete
    void setSavingAllowed(bool);
    void gotGameEnd();
    void gotGameStart();
    void setUnmodifiedListName(QString u)
    {
        unmodifiedListName = u;
    }
    void setHost(QString hostname);

    void gotSpecificCommand(quint8, QByteArray);
    void getAnswerInfo(QByteArray, int);
    void getQuestionInfo(QByteArray, QByteArray, int);
private:
    bool gameGoing;
    bool pendingQuestionData;
    bool pendingAnswerData;
    QString tableHost;
  //  SavedUnscrambleGame currentSug; // TODO delete
    bool savingAllowed;
    QGraphicsScene gfxScene;
    Ui::tableForm tableUi;
    int currentWordLength;
    QList <Tile*> tiles;
    QList <Chip*> chips;
    QList <Chip*> readyChips;
    QList <WordRectangle*> wordRectangles;

    QGraphicsPixmapItem* tableItem;
    QDialog* solutionsDialog;
    Ui::solutionsForm uiSolutions;

    QWidget* preferencesWidget;
    Ui::tableCustomizationForm uiPreferences;

    QSet <quint32> overallQuestionSet;

    struct wordQuestion
    {
        wordQuestion(QString a, QStringList s, quint8 n)
        {
            alphagram = a;
            solutions = s;
            numNotYetSolved = n;
        };

        QString alphagram;
        QStringList solutions;
        quint8 numNotYetSolved;
        QSet <quint8> notYetSolved;
        Chip* chip;
        QList <Tile*> tiles;
        quint8 space;
        quint32 probIndex;
    };

    QHash <QString, int> answerHash;
    QList <wordQuestion> wordQuestions;
    QSet <QString> rightAnswers;
    double verticalVariation;
    double heightScale;
    void loadUserPreferences();
    void swapXPos(Tile*, Tile*);
    int getTileWidth(int wordLength);
    void getBasePosition(int index, double& x, double& y, int tileWidth);

    void submitCorrectAnswer(quint8 space, quint8 specificAnswer);

    QString unmodifiedListName;
    bool savedGameModified;
    PacketBuilder* pb;

protected:
    virtual void closeEvent(QCloseEvent*);
signals:
    void giveUp();
    void sendStartRequest();

    void chatTable(QString);
    void sendPM(QString);
    void exitThisTable();
    void viewProfile(QString);
    void saveCurrentGame();
    void setTablePrivate(bool);
    void showInviteDialog();

    void bootFromTable(QString);

    void requestQuestionData(QByteArray, QString, int);

        private slots:
    void enteredGuess();
    void enteredChat();
    void saveGame();
    void exitButtonPressed();

    void alphagrammizeWords();
    void shuffleWords();
    void tileWasClicked();
    void rectangleWasClicked();
    void setZoom(int);
    void changeTileColors(int);
    void changeFontColors(int);
    void changeTableStyle(int);
    void changeTileBorderStyle(bool);
    void changeVerticalVariation(bool);
    void changeBackground(int index);
    void drawWordBorders(bool);

    void changeTileAspectRatio(bool);
    void changeUseTiles(bool);
    void useFixedWidthFontForRectangles(bool);
    void pushedFontToggleButton();

    void saveUserPreferences();
    void showBootDialog();

    void changeTablePrivacy(int);
        void toggleFullScreen();


};



#endif
