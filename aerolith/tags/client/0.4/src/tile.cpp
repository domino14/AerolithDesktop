#include "tile.h"

Tile::Tile()
{
	QLinearGradient linearGrad(QPointF(0, 0), QPointF(18, 18));
	linearGrad.setColorAt(0, QColor(7, 9, 184).lighter(200));			// 7 9 184
	linearGrad.setColorAt(1, QColor(55, 75, 175).lighter(200));			// 55 75 175
	
	//linearGrad.setColorAt(0, QColor(255, 255, 255));
	//linearGrad.setColorAt(1, QColor(255, 255, 255));

	tileBrush = QBrush(linearGrad);

	//foregroundPen = QPen(Qt::white);
	foregroundPen = QPen(Qt::black);

	edgePen = QPen(Qt::black, 2);
	//edgePen = QPen(Qt::white, 2);
	width = 19;
	//setFlag(QGraphicsItem::ItemIsMovable);
}

void Tile::setWidth(int w)
{
	width = w;
}

QRectF Tile::boundingRect() const
{
	return QRectF(0, 0, width, width);
}

void Tile::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);

	painter->setPen(QPen(tileBrush, 0));
	painter->setBrush(tileBrush);
    painter->drawRect(0, 0, width-1, width-1);

	// draw black shadow
	painter->setPen(edgePen);
	painter->drawLine(1, width-1, width-1, width-1);
	painter->drawLine(width-1, width-1, width-1, 1);
    // Draw text

#ifdef Q_WS_MAC
	QFont font("Courier", width+4, 75);
#else
	QFont font("Courier", width-1, 100);
#endif
//	font.setStyleStrategy(QFont::PreferAntialias);
	painter->setFont(font);
	painter->setPen(foregroundPen);
#ifdef Q_WS_MAC
	painter->drawText(QRectF(0, 0, width-2, width-2), Qt::AlignCenter, tileLetter);
#else
	painter->drawText(QRectF(0,	1, width-2, width-2), Qt::AlignCenter, tileLetter);
#endif   
}

void Tile::setTileProperties(QBrush& b, QPen& p, QPen &e)
{
	tileBrush = b;
	foregroundPen = p;
	edgePen = e;
}

void Tile::setTileBrush(QBrush& b)
{
	tileBrush = b;
}

QBrush Tile::getTileBrush()
{
	return tileBrush;
}

void Tile::setForegroundPen(QPen& p)
{
	foregroundPen = p;
}

void Tile::setEdgePen(QPen &e)
{
	edgePen = e;
}

void Tile::setTileLetter(QString tileLetter)
{
	this->tileLetter = tileLetter;

}

void Tile::mousePressEvent ( QGraphicsSceneMouseEvent * event)
{
	emit mousePressed();
}
