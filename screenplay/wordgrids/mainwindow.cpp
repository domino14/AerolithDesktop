//    Copyright 2007, 2008, 2009, Cesar Del Solar  <delsolar@gmail.com>
//    This file is part of Wordgrids.
//
//    Wordgrids is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Wordgrids is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Wordgrids.  If not, see <http://www.gnu.org/licenses/>.

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>


int scoresByLength[16] =
{ 0, 0, 0, 9, 16, 25, 49, 64, 81, 100, 121, 169, 196, 225, 256, 289 };
/*0  1  2  3  4    5   6   7   8    9   10   11   12   13   14   15*/

int letterDist[26] =
{ 9, 2, 2, 4, 12, 2, 3, 2, 9, 1, 1, 4, 2, 6, 8, 2, 1, 6, 4, 6, 4, 2, 2, 1, 2, 1 };
const int letterDistSum = 98;

QBrush brushUnsolved = QBrush(QColor(255, 200, 150));
QBrush brushSolved = QBrush(QColor(155, 100, 0));
QBrush brushDimmed = QBrush(QColor(155, 100, 50));
QBrush brushHighlight = QBrush(QColor(0, 255, 0));
QBrush brushBonusTile = QBrush(QColor(0, 255, 255));


bool isPerfectSquare(int n)
{
    // Returns true if n is a perfect square
    int i = 0;
    while (i * i < n) i++;
    return i * i == n;
} // isPerfectSquare



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindowClass)
{
    preferencesWidget = new QWidget(this, Qt::Window);
    uiPreferences.setupUi(preferencesWidget);

    definitionPopup = new QWidget(this, Qt::Popup);
    definitionUi.setupUi(definitionPopup);

    loginWidget = new QWidget(this, Qt::Window);
    loginUi.setupUi(loginWidget);



    ui->setupUi(this);
    scene = new WordgridsScene(this);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    ui->graphicsView->viewport()->setFocusPolicy(Qt::ClickFocus);

    tileRect1 = NULL;
    tileRect2 = NULL;


    curselBonusTile = new Tile();
    scene->addItem(curselBonusTile);
    curselBonusTile->setVisible(false);

    curselBonusTile->setTileBrush(brushBonusTile);
    curselBonusTile->setIsBonusTile(true);
    curselBonusTile->setPos(-60, 250);


    scoreLabel = new QGraphicsSimpleTextItem("Score");
    gameScore = new QGraphicsSimpleTextItem();

    scene->addItem(scoreLabel);
    scene->addItem(gameScore);
    scoreLabel->setFont(QFont("Comic Sans", 18, QFont::Bold));
    scoreLabel->setPos(-80, 0);

    gameScore->setFont(QFont("Comic Sans", 16, QFont::DemiBold));
    gameScore->setPos(-60, 40);

    bonusTileLabel = new QGraphicsSimpleTextItem("Bonus");
    bonusTileLabel->setVisible(false);
    scene->addItem(bonusTileLabel);
    bonusTileLabel->setFont(QFont("Comic Sans", 16, QFont::Bold));
    bonusTileLabel->setPos(-80, 210);

    dashInstructionsLabel = new QGraphicsSimpleTextItem("");
    dashInstructionsLabel->setVisible(false);
    scene->addItem(dashInstructionsLabel);
    dashInstructionsLabel->setFont(QFont("Comic Sans", 16, QFont::Bold));
    dashInstructionsLabel->setPos(-110, 300);

    thisScoreLabel = new QGraphicsSimpleTextItem("");
    thisScoreLabel->setZValue(1);
    thisScoreLabel->setVisible(false);
    scene->addItem(thisScoreLabel);
    thisScoreLabel->setFont(QFont("Comic Sans", 36, QFont::Bold));

    bonusScoreLabel = new QGraphicsSimpleTextItem("");
    bonusScoreLabel->setZValue(1);
    bonusScoreLabel->setVisible(false);
    scene->addItem(bonusScoreLabel);
    bonusScoreLabel->setFont(QFont("Comic Sans", 20, QFont::Bold));


    curTileWidth = 37;
    boardWidth = 10;
    boardHeight = 10;

    lastTimerSecs = 0;

    clickState = NO_TILES_CLICKED;

    qsrand(QDateTime::currentDateTime().toTime_t());
    gameTimer.setInterval(1000);
    curScore = 0;

    //ui->textEdit->document()->setMaximumBlockCount(500);
    connect(&gameTimer, SIGNAL(timeout()), SLOT(secPassed()));

    gameGoing = false;
    numSolvedLetters = 0;
    lastGridSize = 10;

    connect(scene, SIGNAL(sceneMouseClicked(double, double)), SLOT(sceneMouseClicked(double, double)));
    connect(scene, SIGNAL(keyPressed(int)), SLOT(keyPressed(int)));

    bonusTilesAllowed = false;

    wordStructure = new WordStructure(this);
    loadedWordStructure = false;
    connect(wordStructure, SIGNAL(finishedLoadingWordStructure()), this, SLOT(finishedLoadingWordStructure()));


    shouldLoadNextNewGame = false;

    scoreLabel->setVisible(false);


    readSettings();
    singleScoreTimer = new QTimer(this);
    singleScoreTimer->setSingleShot(true);
    connect(singleScoreTimer, SIGNAL(timeout()), SLOT(hideSingleScore()));

    bonusScoreTimer = new QTimer(this);
    bonusScoreTimer->setSingleShot(true);
    connect(bonusScoreTimer, SIGNAL(timeout()), SLOT(hideBonusScore()));

    connect(ui->listWidgetWordList, SIGNAL(itemClicked(QListWidgetItem*)), this,
            SLOT(listWidgetItemClicked(QListWidgetItem*)));


    //  ui->listWidgetWordList
    serverCommunicator = new ServerCommunicator(this);
    connect(serverCommunicator, SIGNAL(serverConnectionError(QString)),
            this, SLOT(serverConnectionError(QString)));
    connect(serverCommunicator, SIGNAL(serverDisconnect()),
            this, SLOT(serverDisconnected()));
    connect(serverCommunicator, SIGNAL(serverConnect()),
            this, SLOT(serverConnected()));
    connect(serverCommunicator, SIGNAL(showError(QString)),
            this, SLOT(showServerError(QString)));
    connect(serverCommunicator, SIGNAL(newTable(QByteArray)),
            this, SLOT(newTable(QByteArray)));
    connect(serverCommunicator, SIGNAL(joinTable(QByteArray)),
            this, SLOT(joinTable(QByteArray)));
    connect(serverCommunicator, SIGNAL(chatTable(QByteArray)),
            this, SLOT(gotTableChat(QByteArray)));
    connect(serverCommunicator, SIGNAL(leftTable(QByteArray)),
            this, SLOT(gotLeftTable(QByteArray)));
    connect(serverCommunicator, SIGNAL(timerVal(QByteArray)),
            this, SLOT(gotTimer(QByteArray)));
    connect(serverCommunicator, SIGNAL(curBoard(QByteArray)),
            this, SLOT(gotGameBoard(QByteArray)));
    connect(serverCommunicator, SIGNAL(playerScore(QByteArray)),
            this, SLOT(gotPlayerScore(QByteArray)));
    connect(serverCommunicator, SIGNAL(gameOver(QByteArray)),
            this, SLOT(gotGameOver(QByteArray)));


    connect(ui->lineEditChat, SIGNAL(returnPressed()), this, SLOT(sendTableChat()));
    connect(ui->pushButtonExitTable, SIGNAL(clicked()), this, SLOT(leaveTable()));


    connectedToNetwork = false;
    inATable = false;
    curTable = 0;
    connect(uiPreferences.comboBoxLexicon, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(currentLexiconChanged(QString)));
}

void MainWindow::closeEvent ( QCloseEvent * event )
{
    quit();
}

void MainWindow::quit()
{
    writeSettings();
    QCoreApplication::quit();
}

void MainWindow::mouseOverTile()
{
    return; // no mouse over tile anymore

    // if (uiPreferences.checkBoxAutoHighlightTiles->isChecked() == false) return;
    Tile* tile = qobject_cast<Tile *>(QObject::sender());
    if (clickState == FIRST_TILE_CLICKED)
    {
        if (tile && !tile->isBonusTile())
        {


            int tempX = tile->tileCoordX;
            int tempY = tile->tileCoordY;

            //            QString debugStr = QString("! %1 %2 %3 %4").arg(x1).arg(y1).arg(tempX).arg(tempY);
            //            ui->labelDebug->setText(debugStr);
            highlightedTiles.clear();
            for (int j = qMin(y1, tempY); j <= qMax(y1, tempY); j++)
            {
                for (int i = qMin(x1, tempX); i <= qMax(x1, tempX); i++)
                {


                    int index = j*boardWidth + i;

                    if (tiles.at(index) && tiles.at(index)->getTileLetter() != "")
                    {
                        //tiles.at(index)->setTileBrush(brushBonusActive);

                        highlightedTiles.insert(tiles.at(index));


                    }


                }
            }
            foreach (Tile* tile, tiles)
            {
                if (tile->getTileLetter() == "")
                    if (tile != tileRect1)
                        tile->setTileBrush(brushSolved);
                else
                    tile->setTileBrush(brushHighlight);
                else
                {
                    if (highlightedTiles.contains(tile))
                        tile->setTileBrush(brushHighlight);

                    else
                        tile->setTileBrush(brushUnsolved);
                }

            }
        }




    }
}

void MainWindow::mouseOutOfTile()
{
    return;
    //  if (uiPreferences.checkBoxAutoHighlightTiles->isChecked() == false) return;
    resetTilesHighlightStatus();

}

void MainWindow::resetTilesHighlightStatus()
{
    foreach (Tile* tile, tiles)
    {
        if (tile->getTileLetter() == "")
            tile->setTileBrush(brushSolved);
        else

            tile->setTileBrush(brushUnsolved);
    }
}

void MainWindow::sceneMouseClicked(double x, double y)
{
    if (!gameGoing)
    {
        clickState = NO_TILES_CLICKED;
        return;
    }
    switch (clickState)
    {


    case NO_TILES_CLICKED:
        {
            QGraphicsItem* item = scene->itemAt(x, y);
            Tile* tile = dynamic_cast<Tile *>(item);
            if (tile && !tile->isBonusTile())
            {


                x1 = tile->tileCoordX;
                y1 = tile->tileCoordY;
                clickState = FIRST_TILE_CLICKED;
                tile->setTileBrush(brushHighlight);
                tileRect1 = tile;
            }

            break;
        }
    case FIRST_TILE_CLICKED:

        QGraphicsItem* item = scene->itemAt(x, y);
        Tile* tile = dynamic_cast<Tile *>(item);
        if (tile && !tile->isBonusTile())
        {
            x2 = tile->tileCoordX;
            y2 = tile->tileCoordY;
            tile->setTileBrush(brushHighlight);

            clickState = NO_TILES_CLICKED;
            possibleRectangleCheck();

            resetTilesHighlightStatus();

        }
        break;

    }


}

void MainWindow::keyPressed(int keyCode)
{
    if (bonusTilesAllowed)
    {
        if (keyCode >= Qt::Key_A && keyCode <= Qt::Key_Z)
        {
            char c = keyCode - Qt::Key_A + 'A';
            if (c != 'Q')
                curselBonusTile->setTileLetter(QString(c));
            else
                curselBonusTile->setTileLetter("Qu");
        }

    }

    if (keyCode == Qt::Key_Escape)
    {
        curselBonusTile->setTileLetter("");
        clickState = NO_TILES_CLICKED;
        resetTilesHighlightStatus();
    }


    // ui->textEdit->append("Key was pressed: " + QString::number(keyCode));
}



void MainWindow::possibleRectangleCheck()
{
    // check the rectangle formed from x1, y1 to x2, y2 for alphagram

    if (x1 == x2 && y1 == y2) return;

    QString letters;

    for (int j = qMin(y1, y2); j <= qMax(y1, y2); j++)
    {
        for (int i = qMin(x1, x2); i <= qMax(x1, x2); i++)
        {

            int index = j*boardWidth + i;
            letters += tiles.at(index)->getTileLetter();

        }
    }
    if (letters.size() < 2) return;

    QString debugStr = QString("%1 %2 %3 %4 %5").arg(x1).arg(y1).arg(x2).arg(y2).arg(letters);
    // ui->labelDebug->setText(debugStr);

    int thisScore = 0;
    QString bonusTile;
    if (currentGameDescription == "WordStruck")
    {
        int numExtraLetters = 0;
        bool bonusS = false;
        if (curselBonusTile) bonusTile = curselBonusTile->getTileLetter();
        if (bonusTile != "")
        {
            letters += bonusTile;
            numExtraLetters = bonusTile.length();

            if (bonusTile == "S") bonusS = true;
        }
        if (curselBonusTile) curselBonusTile->setTileLetter("");

        letters = alphagrammize(letters.toUpper());


        /* look for this string in the db*/

        if (wordStructure->wordStructure.contains(letters))
        {
            // this rectangle contains a word!
            QString answers = wordStructure->wordStructure.value(letters);
            ui->listWidgetWordList->insertItem(0, answers);
            for (int j = qMin(y1, y2); j <= qMax(y1, y2); j++)
            {
                for (int i = qMin(x1, x2); i <= qMax(x1, x2); i++)
                {

                    int index = j*boardWidth + i;
                    // todo right here, set a flag on the tile telling it to not be considered in future
                    // rectangles, then write flashing code in a timer somewhere. so that rectangle
                    // flashes and disappears.
                    tiles.at(index)->setTileLetter("");
                    tiles.at(index)->setTileBrush(brushSolved);
                    simpleGridRep[i][j] = ' ';
                }
            }

            int wordLength = letters.size();
            /* handle special QU case */
            if (letters.contains("Q")) wordLength--;    // Q only exists with U. this is a dirty hack.
            if (wordLength <= 15)
            {
                int effectiveWordLength = wordLength;

                if (bonusS)
                    if (wordStructure->containsOnlyPlurals(answers))
                        effectiveWordLength--;  // remove bonus pts for trivially adding an S to make a plural
                thisScore = scoresByLength[effectiveWordLength];
                solvedWordsByLength[wordLength]++;
            }
            else
                thisScore = 400;


            numSolvedLetters += (wordLength - numExtraLetters);


            showSingleScore(thisScore, x1, x2, y1, y2);



            curScore += thisScore;



            if (numSolvedLetters == boardWidth * boardHeight)
            {
                // cleared whole board

                int clearBonus = boardWidth * boardHeight * 2;
                curScore += clearBonus;
                //  ui->textEdit->append(QString("Got +%1 for clearing whole board! Good job!").arg(clearBonus));
                int timeBonus = timerSecs;
                //  ui->textEdit->append(QString("Got %1 for time left!").arg(timeBonus));
                showBonusScore(clearBonus, timeBonus, 1, 1);
                curScore += timeBonus;
                timerSecs = 0;
                gameGoing = false;
            }

            /* deal with bonus tiles */
            if (numSolvedLetters > 0 && numSolvedLetters < (boardWidth*boardHeight) - bonusTurnoffTiles)
            {

                if (!bonusTilesAllowed)
                {
                    bonusTilesAllowed = true;
                    curselBonusTile->setVisible(true);
                    bonusTileLabel->setVisible(true);
                }
            }
            else
            {
                if (bonusTilesAllowed)      // this makes this message only pop up once
                {
                    curselBonusTile->setVisible(false);
                    bonusTileLabel->setVisible(false);

                    // todo replaced with gfx
                    /* ui->textEdit->append(QString("Turned off bonus tiles with %1 tiles left!").
                                         arg(boardWidth*boardHeight - numSolvedLetters));
*/
                    bonusTilesAllowed = false;
                }
            }


            displayScore(curScore);

            scene->update();
            if (connectedToNetwork)
                serverCommunicator->sendCurrentPosition(qMin(x1, x2), qMin(y1, y2),
                                                        qMax(x1, x2), qMax(y1, y2),
                                                        curScore, bonusTile.toAscii());

        }
        else
        {
            if (!connectedToNetwork)
            {
                int penalty = 1;
                timerSecs -= penalty;
                if (penalty > 0) /*ui->textEdit->append(QString("<font color=red>%1-second penalty!</font>").
                                                      arg(penalty));*/
                    showSingleScore(-penalty, x1, x2, y1, y2, true);
            }
            else
            {
                // no penalties when connected to network!?
            }
        }

    }
    else if (currentGameDescription == "WordDash")
    {
        letters = alphagrammize(letters.toUpper());
        int wordLength = letters.size();

        if (foundAlphaset.contains(letters))
        {
            solvedWordsByLength[wordLength]++;
            foundAlphaset.remove(letters);
            solvedAlphaset.insert(letters);
            if (wordLength <= 15)
            {
                thisScore = scoresByLength[wordLength];
            }
            else
                thisScore = 400;

            showSingleScore(thisScore, x1, x2, y1, y2);

            curScore += thisScore;

            markInWordList(wordStructure->wordStructure.value(letters));
            if (wordLength == curGenerating) curSolved++;

            if (curSolved == curToSolve)
            {
                curSolved = 0;
                curGenerating++;

                bool retVal = populateNextFindList();

                if (!retVal)
                {
                    // game over!
                    int clearBonus = boardWidth * boardHeight * 2;
                    curScore += clearBonus;
                    //   ui->textEdit->append(QString("Got +%1 for clearing whole board! Good job!").arg(clearBonus));
                    int timeBonus = timerSecs;
                    //   ui->textEdit->append(QString("Got %1 for time left!").arg(timeBonus));
                    showBonusScore(clearBonus, timeBonus, 1, 1);
                    curScore += timeBonus;
                    timerSecs = 0;
                    gameGoing = false;
                    dashInstructionsLabel->setVisible(false);
                }

            }

            displayScore(curScore);
            if (connectedToNetwork)
                serverCommunicator->sendCurrentPosition(qMin(x1, x2), qMin(y1, y2),
                                                        qMax(x1, x2), qMax(y1, y2),
                                                        curScore, "");  // no bonus tile
            scene->update();


        }
        else if (wordStructure->wordStructure.contains(letters))
        {
            // no penalty

        }
        else
        {
            if (!connectedToNetwork)
            {
                int penalty =1;
                timerSecs -= penalty;
                if (penalty > 0) /*ui->textEdit->append(QString("<font color=red>%1-second penalty!</font>").
                                                      arg(penalty));*/
                    showSingleScore(-penalty, x1, x2, y1, y2, true);
            }
            else
            {
                // no penalty?
            }

        }
    }

}

void MainWindow::showSingleScore(int score, int x1, int x2, int y1, int y2, bool penalty)
{

    double posX = curTileWidth*(double)(x1 + x2)/2.0;
    double posY = curTileWidth*(double)(y1+y2)/2.0;
    thisScoreLabel->setPos(posX, posY);
    thisScoreLabel->setVisible(true);

    if (penalty)
        thisScoreLabel->setBrush(QBrush(Qt::red));
    else
        thisScoreLabel->setBrush(QBrush(Qt::blue));

    if (score >= 0)
        thisScoreLabel->setText(QString("+%1").arg(score));
    else
        thisScoreLabel->setText(QString::number(score) + " s.");

    singleScoreTimer->start(500);


}

void MainWindow::hideSingleScore()
{

    thisScoreLabel->setVisible(false);
}

void MainWindow::showBonusScore(int clearBonus, int timeBonus, int x, int y)
{
    bonusScoreLabel->setPos(x, y);
    bonusScoreLabel->setVisible(true);

    bonusScoreLabel->setBrush(QBrush(Qt::green));

    bonusScoreLabel->setText(QString("+%1 Clear Board Bonus\n+%2 Time Bonus").arg(clearBonus).arg(timeBonus));
    bonusScoreTimer->start(3000);
}

void MainWindow::hideBonusScore()
{

    bonusScoreLabel->setVisible(false);
}

void MainWindow::displayScore(int score)
{
    gameScore->setText(QString::number(score));
}

QString MainWindow::alphagrammize(QString word)
{
    QString ret = word;
    letterList.clear();
    for (int i = 0; i < word.size(); i++)
        letterList << word[i].toLatin1();

    qSort(letterList);

    for (int i = 0; i < letterList.size(); i++)
        ret[i] = letterList[i];

    return ret;
}


void MainWindow::setTilesPos()
{
    foreach (Tile* tile, tiles)
    {

        tile->setPos(tile->tileCoordX * curTileWidth, tile->tileCoordY * curTileWidth);
    }


    scene->setSceneRect(QRectF(0, 0, curTileWidth*boardWidth, curTileWidth*boardHeight));
}

void MainWindow::on_pushButtonGiveUp_clicked()
{
    timerSecs = 0;
}

void MainWindow::on_pushButtonRetry_clicked()
{
    shouldLoadNextNewGame = true;
    gameToLoad = currentGameCode;
    ui->pushButtonNewGame->animateClick();

}

void MainWindow::on_pushButtonNewGame_clicked()
{
    if (!loadedWordStructure) return;

    if (currentGameDescription == "WordStruck")
    {
        dashInstructionsLabel->setVisible(false);
        if (!connectedToNetwork)
            bonusTurnoffTiles = uiPreferences.spinBoxBonusTurnOff->value();
        else
        {
            if (!inATable)
            {
                QMessageBox::warning(this, "Error!", "You are not in a table!");
                return;
            }
            bonusTurnoffTiles = currentBTOFF;

        }
    }
    else if (currentGameDescription == "WordDash")
    {
        if (!connectedToNetwork)
        {
            minToGenerate = uiPreferences.spinBoxMinWordLengthToFind->value();
            maxToGenerate = uiPreferences.spinBoxMaxWordLengthToFind->value();
            curGenerating = minToGenerate;

            if (minToGenerate > maxToGenerate)
            {
                QMessageBox::warning(this, "Can't start!", "Your minimum length for Word Dash "
                                     "is greater than your maximum length!");
                return;

            }
        }
        else
        {
            int gridSize = sqrt(gameToLoad.length());
            if (gridSize == 7) minToGenerate = 4;
            if (gridSize == 9) minToGenerate = 5;
            if (gridSize == 11) minToGenerate = 6;
            maxToGenerate = minToGenerate + 3;
            curGenerating = minToGenerate;

        }

    }


    int gridSize = 0;
    bool ok;
    if (!shouldLoadNextNewGame)
    {


        gridSize = QInputDialog::getInteger(this, "Grid size?", "Please select a grid size", lastGridSize,
                                            MIN_GRID_SIZE, MAX_GRID_SIZE, 1, &ok);

        if (!ok) return;
    }
    else
    {
        gridSize = sqrt(gameToLoad.length());
    }

    if (currentGameDescription == "WordStruck" && !connectedToNetwork)
    {
        timerSecs = QInputDialog::getInteger(this, "Timer?", "Please select desired timer (seconds)",
                                             gridSize*gridSize*3, 1, 5000, 1, &ok);
        if (!ok) timerSecs = gridSize*gridSize*3;
        lastTimerSecs = timerSecs;
    }


    thisRoundLetters.clear();
    foreach (Tile* tile, tiles)
        tile->deleteLater();
    tiles.clear();

    lastGridSize = gridSize;
    boardHeight = gridSize;
    boardWidth = gridSize;


    bonusTilesAllowed = false;  // bonus tiles are not allowed right at the beginning


    for (int j = 0; j < boardHeight; j++)
    {
        for (int i = 0; i < boardWidth; i++)
        {
            Tile* tile = new Tile();
            tiles << tile;
            scene->addItem(tile);
            tile->setTileCoords(i, j);
            tile->setWidth(curTileWidth, 1);
            tile->setTileBrush(brushUnsolved);
            connect(tile, SIGNAL(mouseOverTile()), SLOT(mouseOverTile()));
            connect(tile, SIGNAL(mouseOutOfTile()), SLOT(mouseOutOfTile()));
            //connect(tile, SIGNAL(keyPressed(int)), SLOT(keyPressed(int)));
            // connect(tile, SIGNAL(mousePressedCorner(int, int)), SLOT(tileMouseCornerClicked(int, int)));
        }
    }
    setTilesPos();
    curselBonusTile->setWidth(curTileWidth, 1);
    curselBonusTile->setVisible(false);
    int index = 0;

    if (!shouldLoadNextNewGame)
    {
        currentGameCode = "";
        foreach (Tile* tile, tiles)
        {
            int letter = qrand()%letterDistSum;
            int accum = 0;
            int lettercounter;
            for (lettercounter = 0; lettercounter < 26; lettercounter++)
            {
                accum += letterDist[lettercounter];
                if (letter < accum) break;
            }
            /* handle special Q case */
            if ((char)lettercounter + 'A' == 'Q')
            {
                tile->setTileLetter("Qu");
                thisRoundLetters << "Qu";
            }
            else
            {
                tile->setTileLetter(QString((char)lettercounter + 'A'));
                thisRoundLetters << QString((char)lettercounter + 'A');
            }
            currentGameCode += ((char)lettercounter + 'A');
            tile->setTileBrush(brushUnsolved);
            int x = index % boardWidth;
            int y = index / boardWidth;
            simpleGridRep[x][y] = (char)lettercounter + 'A';
            index++;
        }

    }
    else
    {

        foreach (Tile* tile, tiles)
        {
            if (gameToLoad[index] == 'Q')
            {
                tile->setTileLetter("Qu");
                thisRoundLetters << "Qu";
            }
            else
            {
                tile->setTileLetter(QString(gameToLoad[index]));
                thisRoundLetters << QString(gameToLoad[index]);
            }
            tile->setTileBrush(brushUnsolved);
            int x = index % boardWidth;
            int y = index / boardWidth;
            simpleGridRep[x][y] = gameToLoad[index].toAscii();
            index++;
        }

        shouldLoadNextNewGame = false;
        currentGameCode = gameToLoad;
    }


    for (int j = 0; j < boardHeight; j++)
    {
        QString line;
        for (int i = 0; i < boardWidth; i++)
        {
            line += simpleGridRep[i][j];
        }
        qDebug() << line;
    }

    if (currentGameDescription == "WordDash")
    {
        generateFindList();
        if (!connectedToNetwork)
        {
            int timePerWord = WordDashSecondsPerWord(boardWidth);
            qDebug() << "Time per word:" << timePerWord;
            timerSecs = qMax(foundAlphaset.size() * timePerWord, 1);
        }

    }

    displayScore(0);

    if (!connectedToNetwork)
    {
        ui->lcdNumber->display(timerSecs);
        gameTimer.start();
    }

    ui->listWidgetWordList->clear();
    gameGoing = true;
    curScore = 0;
    numSolvedLetters = 0;
    //ui->textEdit->append("---------------------------------------------");

    for (int i = 0; i < 16; i++)
        solvedWordsByLength[i] = 0;


    //    scene.update();


    if (currentGameDescription == "WordDash")
        populateNextFindList();

    scoreLabel->setVisible(true);
}

int MainWindow::WordDashSecondsPerWord(int size)
{
    if (size <= 8) return 6;
    if (size == 9) return 7;
    if (size == 10) return 8;
    if (size > 10) return 9;
}

void MainWindow::secPassed()
{
    timerSecs--;
    if (timerSecs <= 0)
    {
        endGameActions();
    }
    ui->lcdNumber->display(qMax(timerSecs, 0));
}

void MainWindow::endGameActions()
{
    gameTimer.stop();
    gameGoing = false;
    // todo replace with gfx
    //  ui->textEdit->append("<font color=red>Time is up!</font>");
    QString summary = "You scored %1 points. Word breakdown by length:";
    summary = summary.arg(curScore);
    for (int i = 0; i < 16; i++)
    {
        if (solvedWordsByLength[i] > 0)
            summary += " <font color=green>" + QString::number(i)
            + "s:</font> " + QString::number(solvedWordsByLength[i]);
    }

    ui->plainTextEditChat->appendHtml("<font color=red>Time is up!</font>");
    ui->plainTextEditChat->appendHtml(summary);
    foreach (Tile* tile, tiles)
    {
        tile->setTileBrush(brushDimmed);
    }

}

void MainWindow::leaveNetworkedGame()
{
    endGameActions();
}

void MainWindow::on_toolButtonMinusSize_clicked()
{
    curTileWidth -=5;
    if (curTileWidth < 5) curTileWidth = 5;
    foreach (Tile* tile, tiles)
        tile->setWidth(curTileWidth, 1);
    //
    //    foreach (Tile* tile, bonusTiles)
    //        tile->setWidth(curTileWidth, 1);

    setTilesPos();
}

void MainWindow::on_toolButtonPlusSize_clicked()
{
    curTileWidth +=5;
    if (curTileWidth >= 60) curTileWidth = 60;
    foreach (Tile* tile, tiles)
        tile->setWidth(curTileWidth, 1);
    //
    //    foreach (Tile* tile, bonusTiles)
    //        tile->setWidth(curTileWidth, 1);

    setTilesPos();
}

MainWindow::~MainWindow()
{
    delete ui;
    foreach (Tile* tile, tiles)
        tile->deleteLater();
    //
    //    foreach (Tile* tile, bonusTiles)
    //        tile->deleteLater();


}

void MainWindow::markInWordList(QString str)
{
    QList<QListWidgetItem *> list = ui->listWidgetWordList->findItems (str, Qt::MatchExactly);
    if (list.size() != 1)
    {
        // must be another word length
        return;
    }

    list[0]->setBackground(QBrush(Qt::blue));
    list[0]->setForeground(QBrush(Qt::white));
    list[0]->setData(Qt::UserRole + 1, true);   // mark as highlighted
    /*
    qDebug() << "marked";
    // scroll down if top half of visible items is highlighted
    for (int i = 0; i < ui->listWidgetWordList->count(); i++)
    {
        qDebug() << ui->listWidgetWordList->visualItemRect(ui->listWidgetWordList->item(i))
                << ui->listWidgetWordList->item(i)->text();
    }
    qDebug() << "Overal rectangle";
    qDebug() << ui->listWidgetWordList->maximumViewportSize();*/


    for (int i = 0; i < ui->listWidgetWordList->count(); i++)
    {
        if (ui->listWidgetWordList->item(i)->data(Qt::UserRole + 1).toBool() == false)
        {
            // this item should be visible in the center at least
            ui->listWidgetWordList->scrollToItem(ui->listWidgetWordList->item(i),
                                                 QAbstractItemView::PositionAtCenter);
            break;
        }
    }

}

void MainWindow::generateFindList()
{
    QTime t;
    t.start();
    /* here generate all possible rectangles with words of certain sizes */


    foundAlphaset.clear();
    solvedAlphaset.clear();
    for (int i = 0; i < lastGridSize; i++)
    {
        for (int j = 0; j < lastGridSize; j++)
        {
            // qDebug() << "i j" << i << j;
            generateSingleFindList(minToGenerate, maxToGenerate, i, j, foundAlphaset);
        }
    }
    qDebug() << "Rectangle generation:" << t.elapsed();

    // ui->textEdit->append(QString("lgs %1 %2").arg(lastGridSize).arg(alphaSet.size()));

    foreach (QString alph, foundAlphaset)
    {
        if (!wordStructure->wordStructure.contains(alph))
            foundAlphaset.remove(alph);
    }
    qDebug() << "Found alphaset" << foundAlphaset;
}

bool MainWindow::populateNextFindList()
{
    // populates the next 'find list' by word length
    // if we have reached 'maxtogenerate' return false
    // else highlight words we have already found that may have been of a higher word length
    ui->listWidgetWordList->clear();

    qDebug() << "in populate" << curGenerating << foundAlphaset;
    while (true)
    {

        curToSolve = 0;
        foreach (QString alph, foundAlphaset)
        {
            if (wordStructure->wordStructure.contains(alph) && alph.length() == curGenerating)
            {

                ui->listWidgetWordList->insertItem(0, wordStructure->wordStructure.value(alph));
                curToSolve++;
            }
        }
        qDebug() << "Cur:" << curGenerating << "to solve:" << curToSolve;
        if (curToSolve == 0)
        {
            curGenerating++;
            qDebug() << "It was 0 " << curGenerating << maxToGenerate;
            if (curGenerating > maxToGenerate) return false;
        }
        else break;
    }

    foreach (QString alph, solvedAlphaset)
    {
        if (alph.size() == curGenerating)
        {
            ui->listWidgetWordList->insertItem(0, wordStructure->wordStructure.value(alph));
            markInWordList(wordStructure->wordStructure.value(alph));
        }
    }

    dashInstructionsLabel->setVisible(true);
    dashInstructionsLabel->setText(QString("Find %1s!").arg(curGenerating));


    curSolved = 0;
    return true;
}

QString MainWindow::extractStringsFromRectangle(int TLi, int TLj, int BRi, int BRj, int minLength, int maxLength)
{
    QString word;
    int lengthSoFar = 0;
    for (int i = TLi; i <= BRi; i++)
    {
        for (int j = TLj; j <= BRj; j++)
        {
            char letter = simpleGridRep[i][j];
            if (letter != ' ')
            {
                lengthSoFar++;
                word += letter;
            }
            if (letter == 'Q')
            {
                // special case, handle QU
                lengthSoFar++;
                word += 'U';
            }
            if (lengthSoFar > maxLength) return ""; // return blank string if the length is already too long.
        }
    }
    if (lengthSoFar >= minLength && lengthSoFar <= maxLength) return word;
    else return "";
}

void MainWindow::generateSingleFindList(int minLength, int maxLength,
                                        int TLi, int TLj, QSet<QString>& alphaSet)
{
    //
    //    lengthSoFar++;
    //    if (lengthSoFar > maxLength) return;
    /* try all squares to the right and bottom as the "bottom right corner" */

    QString curStr;
    int lengthSoFar = 0;
    for (int i = TLi; i < lastGridSize; i++)
    {
        for (int j = TLj; j < lastGridSize; j++)
        {
            QString possibleWord = extractStringsFromRectangle(TLi, TLj, i, j, minLength, maxLength);
            if (possibleWord != "") alphaSet.insert(alphagrammize(possibleWord));
        }
    }

}

void MainWindow::finishedLoadingWordStructure()
{
    loadedWordStructure = true;
    ui->statusBar->showMessage("Loaded word structure! You may begin playing.");
    ui->pushButtonNewGame->setEnabled(true);
    uiPreferences.comboBoxLexicon->setEnabled(true);
}

void MainWindow::on_actionLoad_board_triggered()
{
    if (connectedToNetwork)
    {
        QMessageBox::warning(this, "Error", "You can only load a game when you are offline.");
        return;
    }
    QString code = QInputDialog::getText(this, "Enter code", "Enter passcode");
    if (code != "" && isPerfectSquare(code.length()))
    {
        shouldLoadNextNewGame = true;
        gameToLoad = code.simplified().toUpper();
        ui->pushButtonNewGame->animateClick();
    }
    else
    {
        QMessageBox::warning(this, "Code incorrect!", "Code incorrect");
    }
}

void MainWindow::on_actionSave_board_triggered()
{
    if (currentGameCode != "")
    {

        QMessageBox::information(this, "Code", "Use the following code when you click 'Load game':<BR>"
                                 + currentGameCode + "<BR><BR>This code has been copied to the clipboard."
                                 "<BR>Press Ctrl + V in a new text file if you wish to save it.");
        QApplication::clipboard()->setText(currentGameCode);
        //  ui->textEdit->append("Use the following code when you click 'Load game':");
        // ui->textEdit->append(currentGameCode);
    }
}

void MainWindow::on_actionEdit_game_preferences_triggered()
{
    preferencesWidget->show();
}

void MainWindow::writeSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);

    settings.beginGroup("WordDash");
    settings.setValue("MinWordLengthToFind", uiPreferences.spinBoxMinWordLengthToFind->value());
    settings.setValue("MaxWordLengthToFind", uiPreferences.spinBoxMaxWordLengthToFind->value());
    settings.endGroup();

    settings.beginGroup("WordStruck");
    settings.setValue("BonusTileTurnoff", uiPreferences.spinBoxBonusTurnOff->value());

    settings.endGroup();

    settings.beginGroup("GeneralSettings");
    settings.setValue("DefaultGame", currentGameDescription);
    settings.setValue("DefaultUsername", loginUi.lineEditUsername->text());
    settings.setValue("DefaultLexiconIndex", uiPreferences.comboBoxLexicon->currentIndex());
    settings.endGroup();
}

void MainWindow::readSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);

    settings.beginGroup("WordDash");
    uiPreferences.spinBoxMaxWordLengthToFind->setValue(settings.value("MaxWordLengthToFind", 9).toInt());
    uiPreferences.spinBoxMinWordLengthToFind->setValue(settings.value("MinWordLengthToFind",6).toInt());
    settings.endGroup();

    settings.beginGroup("WordStruck");
    uiPreferences.spinBoxBonusTurnOff->setValue(settings.value("BonusTileTurnoff", 10).toInt());
    settings.endGroup();

    settings.beginGroup("GeneralSettings");
    currentGameDescription = settings.value("DefaultGame", "WordStruck").toString();
    loginUi.lineEditUsername->setText(settings.value("DefaultUsername", "").toString());
    uiPreferences.comboBoxLexicon->setCurrentIndex(settings.value("DefaultLexiconIndex", 0).toInt());
    // trigger the loading of the lexicon
    currentLexiconChanged(uiPreferences.comboBoxLexicon->currentText());
    settings.endGroup();

    QString switchStr;
    if (currentGameDescription == "WordStruck") switchStr = "WordDash";
    else if (currentGameDescription == "WordDash") switchStr = "WordStruck";

    ui->pushButtonSwitchGame->setText("Switch to: " + switchStr);
    ui->labelCurrentGame->setText(currentGameDescription);

}

void MainWindow::on_pushButtonSwitchGame_clicked()
{
    if (gameGoing)
        QMessageBox::information(this, "Can't switch", "Your current game must end before you can switch games!");
    else
    {

        QString switchStr;
        if (currentGameDescription == "WordStruck")
        {
            currentGameDescription = "WordDash";
            switchStr = "WordStruck";
        }
        else if (currentGameDescription == "WordDash")
        {
            currentGameDescription = "WordStruck";
            switchStr = "WordDash";

        }
        ui->pushButtonSwitchGame->setText("Switch to: " + switchStr);

        ui->labelCurrentGame->setText(currentGameDescription);
    }

}

void MainWindow::listWidgetItemClicked(QListWidgetItem* it)
{
    if (it->data(Qt::UserRole).toString() == "")
    {
        it->setData(Qt::UserRole, wordStructure->getDefinitions(it->text()));
    }
    QString text = it->data(Qt::UserRole).toString();
    int nLineBreaks = qMax(text.count('\n'), 2);
    definitionUi.plainTextEdit->setPlainText(it->data(Qt::UserRole).toString());
    definitionPopup->move(ui->listWidgetWordList->x()+this->x(), ui->listWidgetWordList->y()+this->y());
    definitionPopup->setFixedHeight(nLineBreaks*25 + 50);
    definitionPopup->show();
}

void MainWindow::on_actionConnectToServer_triggered()
{
    loginWidget->show();
}

void MainWindow::on_pushButtonConnect_clicked()
{


    if (!serverCommunicator->isConnectedToServer())
    {

        serverCommunicator->connectToServer(loginUi.lineEditServer->text(), loginUi.lineEditPort->text().toInt(),
                                            loginUi.lineEditUsername->text());
        myUsername = loginUi.lineEditUsername->text().toAscii();
        ui->labelOnlineStatus->setText("Connecting to server...");
        loginUi.pushButtonConnect->setText("Disconnect");


        ui->listWidgetTables->clear();
    }
    else
    {
        serverCommunicator->disconnectFromServer();
        performDisconnectedActions();

    }
}

void MainWindow::performDisconnectedActions()
{
    qDebug() << "Performing disconnected actions.";
    ui->labelOnlineStatus->setText("<font color=red>Disconnected from server</font>");
    ui->listWidgetTables->clear();
    ui->stackedWidget->setCurrentIndex(0);

    loginUi.pushButtonConnect->setText("Connect");
    ui->stackedWidget->setCurrentIndex(0);

    loginWidget->show();
    tablesHash.clear();
    inATable = false;
    curTable = 0;
    connectedToNetwork = false;
    /* re-enable buttons */
    ui->pushButtonNewGame->setEnabled(true);
    ui->pushButtonRetry->setEnabled(true);
    ui->pushButtonGiveUp->setEnabled(true);
    ui->pushButtonSwitchGame->setEnabled(true);
    uiPreferences.comboBoxLexicon->setEnabled(true);
}

void MainWindow::serverConnectionError(QString error)
{
    QMessageBox::information(this, "Error", error);
    loginUi.pushButtonConnect->setText("Connect!");
    ui->labelOnlineStatus->setText("<font color=red>Disconnected.</font>");
}

void MainWindow::showServerError(QString error)
{
    QMessageBox::information(this, "Error", error);
}

void MainWindow::serverDisconnected()
{
    ui->labelOnlineStatus->setText("<font color=red>You are disconnected.</font>");
    performDisconnectedActions();
}

void MainWindow::serverConnected()
{
    timerSecs = 0;  // thus ending any current non-networked game going on
    ui->labelOnlineStatus->setText("<font color=green>Connected to server!</font>");
    ui->listWidgetTables->clear();
    ui->stackedWidget->setCurrentIndex(0);
    loginUi.pushButtonConnect->setText("Disconnect");
    loginWidget->hide();
    tablesHash.clear();
    connectedToNetwork = true;

    /* disable buttons */
    ui->pushButtonNewGame->setEnabled(false);
    ui->pushButtonRetry->setEnabled(false);
    ui->pushButtonGiveUp->setEnabled(false);
    ui->pushButtonSwitchGame->setEnabled(false);
    uiPreferences.comboBoxLexicon->setEnabled(false);
}

void MainWindow::newTable(QByteArray ba)
{
    QList <QByteArray> params = ba.split(' ');
    int tablenum = params[0].toInt();
    int gridSize = params[1].toInt();
    int btTurnoff = params[2].toInt();
    QString type = params[3];
    int numPpl = params[4].toInt();
    QString label;
    if (type == "STRUCK")
        label = QString("%1: %2 x %2, BTOFF @ %3, (%4 players)").
                arg(type).arg(gridSize).arg(btTurnoff).arg(numPpl);
    else if (type == "DASH")
        label = QString("%1: %2 x %2 (%3 players)").arg(type).arg(gridSize).arg(numPpl);
    QListWidgetItem* lwi = new QListWidgetItem(label, ui->listWidgetTables);
    lwi->setData(Qt::UserRole + 1, tablenum);
    lwi->setData(Qt::UserRole + 2, btTurnoff);
    tablesHash.insert(tablenum, lwi);

}

void MainWindow::joinTable(QByteArray ba)
{

    QList <QByteArray> params = ba.split(' ');
    int tablenum = params[1].toInt();
    QByteArray username = params[0];
    qDebug() << username << "joined" << tablenum;
    if (tablesHash.contains(tablenum))
    {
        QListWidgetItem* lwi = tablesHash.value(tablenum);
        QList <QVariant> users = lwi->data(Qt::UserRole).toList();
        users << QVariant(username);
        lwi->setData(Qt::UserRole, users);
        int numP = users.size();
        int index = lwi->text().indexOf("(");
        QString before = lwi->text().left(index);
        QString tableText = before + QString("(%1 players)").arg(numP);
        lwi->setText(tableText);


        if (tablenum == curTable && username != myUsername)
        {
            // if i'm aready in this table and someone else joined
            PlayerListWidgetItem* item = new PlayerListWidgetItem();
            item->setScore(0);
            item->setUsername(username);
            item->setText(item->formattedText());
            ui->listWidgetMyTable->addItem(item);

        }

        if (username == myUsername)
        {
            ui->labelTableName->setText(tableText);
            inATable = true;
            ui->listWidgetMyTable->clear();
            ui->plainTextEditChat->clear();
            populateTableList(lwi);
            ui->stackedWidget->setCurrentIndex(1);  // switch to table view
            curTable = tablenum;

            if (tableText.contains("STRUCK")) currentGameDescription = "WordStruck";
            else if (tableText.contains("DASH")) currentGameDescription = "WordDash";

            QString switchStr;
            if (currentGameDescription == "WordStruck") switchStr = "WordDash";
            else if (currentGameDescription == "WordDash") switchStr = "WordStruck";

            ui->pushButtonSwitchGame->setText("Switch to: " + switchStr);
            ui->labelCurrentGame->setText(currentGameDescription);

            currentBTOFF = lwi->data(Qt::UserRole + 2).toInt();
        }

    }
}



void MainWindow::populateTableList(QListWidgetItem * item)
{
    QList <QVariant> people = item->data(Qt::UserRole).toList();
    foreach (QVariant person, people)
    {
        PlayerListWidgetItem* item = new PlayerListWidgetItem();
        item->setScore(0);
        item->setUsername(person.toByteArray());
        item->setText(item->formattedText());
        ui->listWidgetMyTable->addItem(item);
    }
}

void MainWindow::on_pushButtonJoinTable_clicked()
{
    // TODO if not already in a table
    if (!inATable)
    {
        QListWidgetItem* lwi = ui->listWidgetTables->currentItem();
        if (lwi)
            serverCommunicator->sendJoinTable(lwi->data(Qt::UserRole + 1).toInt());
    }
}

void MainWindow::sendTableChat()
{
    QByteArray chat = ui->lineEditChat->text().left(255).simplified().toAscii();
    serverCommunicator->sendToTable(chat);
    ui->lineEditChat->clear();
}

void MainWindow::gotTableChat(QByteArray ba)
{
    QList <QByteArray> params = ba.split(' ');
    // tablenum, user, chat
    int tablenum = params[0].toInt();
    QByteArray user = params[1];
    if (tablenum != curTable) return;
    QByteArray chat;
    for (int i = 2; i < params.size(); i++)
    {
        chat += params[i] + " ";
    }
    ui->plainTextEditChat->appendPlainText(user + ": " + chat);
}

void MainWindow::leaveTable()
{
    serverCommunicator->sendLeaveTable();

}

void MainWindow::gotTimer(QByteArray ba)
{
    QList <QByteArray> params = ba.split(' ');
    int tablenum = params[0].toInt();
    int timer = params[1].toInt();
    if (tablenum == curTable)
        ui->lcdNumber->display(timer);
    timerSecs = timer;  // used for calculating bonus scores
}

void MainWindow::gotLeftTable(QByteArray ba)
{
    // username tablenum
    QList <QByteArray> params = ba.split(' ');
    int tablenum = params[1].toInt();
    QByteArray username = params[0];
    if (tablesHash.contains(tablenum))
    {
        QListWidgetItem* lwi = tablesHash.value(tablenum);
        QList <QVariant> users = lwi->data(Qt::UserRole).toList();
        users.removeAll(QVariant(username));
        lwi->setData(Qt::UserRole, users);
        int numP = users.size();
        int index = lwi->text().indexOf("(");
        QString before = lwi->text().left(index);
        lwi->setText(before + QString("(%1 players)").arg(numP));


        if (tablenum == curTable && username != myUsername)
        {
            // if i'm aready in this table and someone else left

            QListWidgetItem* it = findPlayerInTable(username);
            if (it) delete it;

        }

        if (username == myUsername)
        {
            inATable = false;
            ui->stackedWidget->setCurrentIndex(0);  // switch to table list view
            curTable = 0;
            leaveNetworkedGame();
        }

    }
}

PlayerListWidgetItem* MainWindow::findPlayerInTable(QByteArray username)
{
    for (int i = 0; i < ui->listWidgetMyTable->count(); i++)
    {
        PlayerListWidgetItem* plwi = dynamic_cast<PlayerListWidgetItem*>(ui->listWidgetMyTable->item(i));
        if (plwi && plwi->getUsername() == username) return plwi;
        if (!plwi) qDebug("unable to cast");
    }
    return NULL;    // if couldn't find this player
}

void MainWindow::gotGameBoard(QByteArray ba)
{
    QList <QByteArray> params = ba.split(' ');
    qDebug() << "Getting new board" << params[0].toInt() << curTable << params[1];
    if (params[0].toInt() == curTable)
    {
        shouldLoadNextNewGame = true;
        gameToLoad = params[1];
        on_pushButtonNewGame_clicked();
    }

}

void MainWindow::gotPlayerScore(QByteArray ba)
{
    QList <QByteArray> params = ba.split(' ');
    if (params[0].toInt() == curTable)
    {
        int score = params[1].toInt();
        QByteArray player = params[2];
        PlayerListWidgetItem* it = findPlayerInTable(player);
        if (it)
        {
            it->setScore(score);
            it->setText(it->formattedText());
            ui->listWidgetMyTable->sortItems(Qt::DescendingOrder);


        }

    }
}

void MainWindow::gotGameOver(QByteArray ba)
{
    if (ba.toInt() == curTable)
    {
        endGameActions();
    }
}

void MainWindow::currentLexiconChanged(QString text)
{
    uiPreferences.comboBoxLexicon->setEnabled(false);
    ui->pushButtonNewGame->setEnabled(false);
    wordStructure->loadWordStructure(text + ".db");
    // ui->textEdit->append("Please wait for word structure to load...");
    ui->statusBar->showMessage("Please wait... Loading word structure");
    currentLexicon = text;
}

/***********************/

WordgridsScene::WordgridsScene(QObject *parent) : QGraphicsScene(parent)
{


}

void WordgridsScene::mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent )
{
    QGraphicsScene::mousePressEvent(mouseEvent);
    setFocus();
    emit sceneMouseClicked(mouseEvent->scenePos().x(), mouseEvent->scenePos().y());
    //  mouseEvent->ignore();
    // qDebug() << "Scene focus" << hasFocus() << focusItem();
}

void WordgridsScene::keyPressEvent ( QKeyEvent * keyEvent )
{
    QGraphicsScene::keyPressEvent(keyEvent);
    //  qDebug() << "Key pressed: " << keyEvent->key();
    emit keyPressed(keyEvent->key());
}
