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

#ifndef _TILE_H_
#define _TILE_H_

#include <QtGui>
#include <QtCore>

class Tile : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    Tile();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget);
    QRectF boundingRect() const;
    void setTileProperties(QBrush& b, QPen& p, QPen &e);
    void setTileLetter(QString tileLetter);
    void setTileBrush(QBrush& b);
    void setForegroundPen(QPen& p);
    void setEdgePen(QPen& e);
    QBrush getTileBrush();
    void setWidth(int w, double hscale);
    int getWidth();
    void setLexiconForMapping(QString);
private:
    int width;
    int height;
    QFont tileFont;
    QString currentLexicon;
    QBrush tileBrush;
    QPen foregroundPen;
    QPen edgePen;
    QString tileLetter;
    void mousePressEvent ( QGraphicsSceneMouseEvent * event);

signals:
    void mousePressed();
};

#endif
