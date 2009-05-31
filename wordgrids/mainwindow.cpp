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



int letterDist[26] =
{ 9, 2, 2, 4, 12, 2, 3, 2, 9, 1, 1, 4, 2, 6, 8, 2, 1, 6, 4, 6, 4, 2, 2, 1, 2, 1 };
const int letterDistSum = 98;

QBrush brushUnsolved = QBrush(QColor(255, 200, 150));
QBrush brushSolved = QBrush(QColor(155, 100, 0));

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindowClass)
{
    ui->setupUi(this);
    ui->graphicsView->setScene(&scene);

    firstCorner = new QGraphicsPixmapItem();
    secondCorner = new QGraphicsPixmapItem();

    QPixmap xhairspix = QPixmap(":/images/resources/crosshairs.png");
    crossHairsWidth = xhairspix.width();

    firstCorner->setPixmap(xhairspix);
    secondCorner->setPixmap(xhairspix);
    firstCorner->setZValue(1);
    secondCorner->setZValue(1);
    scene.addItem(firstCorner);
    scene.addItem(secondCorner);

    firstCorner->setVisible(false);
    secondCorner->setVisible(false);
    firstCorner->setPos(0,0);
    secondCorner->setPos(0, 0);


    curTileWidth = 40;
    boardWidth = 10;
    boardHeight = 10;


    cornerState = BOTH_CORNERS_OFF;
    qsrand(QDateTime::currentDateTime().toTime_t());
    gameTimer.setInterval(1000);
    curScore = 0;

    ui->textEdit->document()->setMaximumBlockCount(500);
    connect(&gameTimer, SIGNAL(timeout()), SLOT(secPassed()));

    wordDb = QSqlDatabase::addDatabase("QSQLITE");
    wordDb.setDatabaseName("OWL2+LWL.db");
    wordDb.open();
    gameGoing = false;
    numSolvedLetters = 0;
    lastGridSize = 10;
}

void MainWindow::tileMouseCornerClicked(int x, int y)
{
    if (!gameGoing)
    {
        cornerState = BOTH_CORNERS_OFF;
        return;
    }
    switch (cornerState)
    {
    case BOTH_CORNERS_OFF:
        firstCorner->setVisible(true);
        firstCorner->setPos(x * curTileWidth - crossHairsWidth/2, y * curTileWidth - crossHairsWidth/2);
        cornerState = LEFT_CORNER_ON;
        x1 = x; y1 = y;
        break;
    case LEFT_CORNER_ON:
        secondCorner->setVisible(true);
        secondCorner->setPos(x * curTileWidth - crossHairsWidth/2, y * curTileWidth - crossHairsWidth/2);
        firstCorner->setVisible(false);
        secondCorner->setVisible(false);
        x2 = x; y2 = y;
        cornerState = BOTH_CORNERS_OFF;
        possibleRectangleCheck();
        break;

    }

}

void MainWindow::possibleRectangleCheck()
{
    // check the rectangle formed from x1, y1 to x2, y2 for alphagram

    if (x1 == x2 && y1 == y2) return;

    QString letters;
    for (int j = qMin(y1, y2); j < qMax(y1, y2); j++)
    {
        for (int i = qMin(x1, x2); i < qMax(x1, x2); i++)
        {

            int index = j*boardWidth + i;
            letters += tiles.at(index)->getTileLetter();
        }
    }
    if (letters.size() < 2) return;

   // QString debugStr = QString("%1 %2 %3 %4 %5").arg(x1).arg(y1).arg(x2).arg(y2).arg(letters);
   // ui->labelDebug->setText(debugStr);

    letters = alphagrammize(letters);

    /* look for this string in the db*/

    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.prepare("SELECT words from alphagrams where alphagram = ?");
    query.addBindValue(letters);
    query.exec();

    int numResults = 0;
    while (query.next())
    {
        numResults++;
        ui->listWidget->insertItem(0, query.value(0).toString());
        for (int j = qMin(y1, y2); j < qMax(y1, y2); j++)
        {
            for (int i = qMin(x1, x2); i < qMax(x1, x2); i++)
            {

                int index = j*boardWidth + i;
                tiles.at(index)->setTileLetter("");
                tiles.at(index)->setTileBrush(brushSolved);
            }
        }

        scene.update(scene.itemsBoundingRect());
        ui->textEdit->append(QString("+%1 for %2-letter word!").
                             arg(letters.size()*letters.size()).arg(letters.size()));

        numSolvedLetters += letters.size();
        curScore += (letters.size() * letters.size());
        if (numSolvedLetters == boardWidth * boardHeight)
        {
            // cleared whole board
            timerSecs = 0;
            int bonus = boardWidth * boardHeight;
            curScore += bonus;
            ui->textEdit->append(QString("Got +%1 for clearing whole board! Good job!").arg(bonus));
        }


        ui->lcdNumberScore->display(curScore);
    }

    if (numResults > 1)
        ui->listWidget->addItem("ERROR!!!!!!!");
    else if (numResults == 0)
    {
        timerSecs -= 10;
        ui->textEdit->append("<font color=red>10-second penalty!</font>");
    }

    query.exec("END TRANSACTION");
    //    ui->listWidget->addItem(letters);
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

    // tile->setTileLetter(QString(qrand()%26 + 'A'));


}

void MainWindow::on_pushButtonNewGame_clicked()
{
    foreach (Tile* tile, tiles)
        tile->deleteLater();
    tiles.clear();
    bool ok;
    int gridSize = QInputDialog::getInteger(this, "Grid size?", "Please select a grid size", lastGridSize, 2, 20, 1, &ok);

    if (!ok) return;

    lastGridSize = gridSize;
    boardHeight = gridSize;
    boardWidth = gridSize;

    for (int j = 0; j < boardHeight; j++)
    {
        for (int i = 0; i < boardWidth; i++)
        {
            Tile* tile = new Tile();
            tiles << tile;
            scene.addItem(tile);
            tile->setTileCoords(i, j);
            tile->setWidth(curTileWidth, 1);
            tile->setTileBrush(brushUnsolved);
            connect(tile, SIGNAL(mousePressedCorner(int, int)), SLOT(tileMouseCornerClicked(int, int)));
        }
    }
    setTilesPos();


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
        tile->setTileLetter(QString((char)lettercounter + 'A'));
        tile->setTileBrush(brushUnsolved);
    }
    timerSecs = boardWidth*boardHeight*3;
    ui->lcdNumber->display(timerSecs);
    ui->lcdNumberScore->display(0);
    gameTimer.start();
    gameGoing = true;
    curScore = 0;
    numSolvedLetters = 0;
    ui->textEdit->append("---------------------------------------------");

    scene.update(scene.itemsBoundingRect());
}

void MainWindow::secPassed()
{
    timerSecs--;
    if (timerSecs <= 0)
    {
        gameTimer.stop();
        gameGoing = false;
        ui->textEdit->append("<font color=red>Time is up!</font>");
    }
    ui->lcdNumber->display(qMax(timerSecs, 0));
}

void MainWindow::on_toolButtonMinusSize_clicked()
{
    curTileWidth -=5;
    if (curTileWidth < 5) curTileWidth = 5;
    foreach (Tile* tile, tiles)
        tile->setWidth(curTileWidth, 1);

    setTilesPos();
}

void MainWindow::on_toolButtonPlusSize_clicked()
{
    curTileWidth +=5;
    if (curTileWidth >= 60) curTileWidth = 60;
    foreach (Tile* tile, tiles)
        tile->setWidth(curTileWidth, 1);

    setTilesPos();
}

MainWindow::~MainWindow()
{
    delete ui;
    foreach (Tile* tile, tiles)
        tile->deleteLater();


}
