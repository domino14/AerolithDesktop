#ifndef _CHIP_H_
#define _CHIP_H_

#include <QtGui>
#include <QtCore>

class Chip : public QGraphicsItem
{
public:
	Chip();
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget);
	QRectF boundingRect() const;

	void setChipProperties(QBrush& b, QPen& p, QPen &e);
	void setChipNumber(quint8 number);
	void setNumberMode(bool mode);
	void setChipString(QString chipString);
private:
	QBrush chipBrush;
	QPen foregroundPen;
	QPen edgePen;
	quint8 number;
	bool numberMode;
	QString chipString;
};


#endif
