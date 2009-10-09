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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QtGui>
#include <QtCore>
#include "Tile.h"

#include "wordstructure.h"
namespace Ui
{
    class MainWindowClass;
}

class WordgridsScene : public QGraphicsScene
{
    Q_OBJECT
    void mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    void mouseMoveEvent (QGraphicsSceneMouseEvent * mouseEvent );
    void keyPressEvent ( QKeyEvent * keyEvent )  ;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent);
signals:
    void sceneMouseClicked(double, double);
    void sceneMouseMoved(double, double);
    void keyPressed(int);
};

#define MIN_GRID_SIZE 3
#define MAX_GRID_SIZE 20

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();



private:
    enum CornerStates
    {
        LEFT_CORNER_ON, BOTH_CORNERS_ON, BOTH_CORNERS_OFF, BONUS_TILE_SELECTED
    };

    Ui::MainWindowClass *ui;
    WordgridsScene scene;
    Tile* curselBonusTile;
    Tile* requiredBonusTile;
    bool bonusTilesAllowed;
    QVector <Tile*> tiles;
    QVector <Tile*> bonusTiles;
    QGraphicsPixmapItem *firstCorner, *secondCorner;
    QGraphicsLineItem *line1, *line2, *line3, *line4;
    CornerStates cornerState;
    int crossHairsWidth;
    void setTilesPos();
    QTimer gameTimer;
    int timerSecs, lastTimerSecs;
    int curTileWidth;
    int curScore;

    WordStructure* wordStructure;

    bool gameGoing;
    void possibleRectangleCheck();

    int boardWidth, boardHeight;

    int x1, y1, x2, y2;
    QString alphagrammize(QString);
    QList<unsigned char> letterList;
    QList <QString> thisRoundLetters;
    int numSolvedLetters;
    int lastGridSize;
    int solvedWordsByLength[16];
    char simpleGridRep[MAX_GRID_SIZE][MAX_GRID_SIZE];

    void generateFindList();
    void generateSingleFindList(int, int, int, int, QSet<QString>&);
    QString extractStringsFromRectangle(int TLi, int TLj, int BRi, int BRj, int minLength, int maxLength);
    bool loadedWordStructure;

    int minLengthHints, maxLengthHints, bonusTurnoffTiles;
public slots:
    void tileMouseCornerClicked(int, int);
private slots:
    void secPassed();
    void sceneMouseClicked(double, double);
    void sceneMouseMoved(double, double);
    void keyPressed(int);
    void on_pushButtonNewGame_clicked();
    void on_pushButtonRetry_clicked();
    void on_pushButtonGiveUp_clicked();

    void on_toolButtonMinusSize_clicked();
    void on_toolButtonPlusSize_clicked();
    void finishedLoadingWordStructure();

};

#endif // MAINWINDOW_H
