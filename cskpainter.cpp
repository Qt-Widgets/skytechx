#include "cskpainter.h"
#include "const.h"

#include <QDebug>

///////////////////////////////////////////////////////////////
CSkPainter::CSkPainter(QPaintDevice *parent) : QPainter(parent)
///////////////////////////////////////////////////////////////
{
}

////////////////////////
CSkPainter::CSkPainter()
////////////////////////
{
}

//////////////////////////////////////////////////////////////////////////
void CSkPainter::drawCornerBox(int x, int y, int halfSize, int cornerSize)
//////////////////////////////////////////////////////////////////////////
{
  drawLine(x - halfSize, y - halfSize, x - halfSize + cornerSize, y - halfSize);
  drawLine(x - halfSize, y - halfSize, x - halfSize, y - halfSize + cornerSize);

  drawLine(x + halfSize, y - halfSize, x + halfSize - cornerSize, y - halfSize);
  drawLine(x + halfSize, y - halfSize, x + halfSize, y - halfSize + cornerSize);

  drawLine(x - halfSize, y + halfSize, x - halfSize + cornerSize, y + halfSize);
  drawLine(x - halfSize, y + halfSize, x - halfSize, y + halfSize - cornerSize);

  drawLine(x + halfSize, y + halfSize, x + halfSize - cornerSize, y + halfSize);
  drawLine(x + halfSize, y + halfSize, x + halfSize, y + halfSize - cornerSize);
}


//////////////////////////////////////////////////
void CSkPainter::drawCross(int x, int y, int size)
//////////////////////////////////////////////////
{
  drawLine(x, y - size, x, y + size);
  drawLine(x - size, y, x + size, y);
}


//////////////////////////////////////////////
void CSkPainter::drawCross(QPoint p, int size)
//////////////////////////////////////////////
{
  drawLine(p.x(), p.y() - size, p.x(), p.y() + size);
  drawLine(p.x() - size, p.y(), p.x() + size, p.y());
}


//////////////////////////////////////////////////
void CSkPainter::drawCrossX(int x, int y, int size)
//////////////////////////////////////////////////
{
  drawLine(x - size, y - size, x + size, y + size);
  drawLine(x + size, y - size, x - size, y + size);
}

//////////////////////////////////////////////////////////////////
void CSkPainter::drawHalfCross(int x, int y, int size1, int size2)
//////////////////////////////////////////////////////////////////
{
  drawLine(x, y - size1, x, y - size1 + size2);
  drawLine(x, y + size1, x, y + size1 - size2);

  drawLine(x - size1, y, x - size1 + size2, y);
  drawLine(x + size1, y, x + size1 - size2, y);
}


///////////////////////////////////////////////////////
// align lower right
void CSkPainter::drawTextLR(int x, int y, QString text)
///////////////////////////////////////////////////////
{
  drawText(QRect(x, y, 100000, 100000), Qt::AlignTop | Qt::AlignLeft, text);
}


///////////////////////////////////////////////////////
// align upper right
void CSkPainter::drawTextUR(int x, int y, QString text)
///////////////////////////////////////////////////////
{
  drawText(QRect(x, y - 100000, 100000, 100000), Qt::AlignLeft | Qt::AlignBottom, text);
}

///////////////////////////////////////////////////////
// align upper left
void CSkPainter::drawTextUL(int x, int y, QString text)
///////////////////////////////////////////////////////
{
  drawText(QRect(x - 100000, y - 100000, 100000, 100000), Qt::AlignBottom | Qt::AlignRight, text);
}


///////////////////////////////////////////////////////
// align lower left
void CSkPainter::drawTextLL(int x, int y, QString text)
///////////////////////////////////////////////////////
{
  drawText(QRect(x - 100000, y, 100000, 100000), Qt::AlignTop | Qt::AlignRight, text);
}

/////////////////////////////////////////////////////
void CSkPainter::drawCText(int x, int y, QString str)
/////////////////////////////////////////////////////
{
  QRect rc(x - 100000, y - 100000, 200000, 200000);

  drawText(rc, Qt::AlignCenter, str);
}

//////////////////////////////////////////////////////////////////////////////////
void CSkPainter::drawRotatedText(float degrees, int x, int y, const QString &text)
//////////////////////////////////////////////////////////////////////////////////
{
  save();
  translate(x, y);
  rotate(degrees);
  drawText(0, 0, text);
  restore();
}

QRect CSkPainter::renderText(int x, int y, double offset, const QString &text, int align, bool render)
{
  QFontMetrics fm(font());
  QRect trc = fm.boundingRect(text);

  trc.moveLeft(x);
  trc.moveBottom(y);

  switch (align)
  {
    case RT_CENTER:
    {
      trc.moveCenter(QPoint(x, y));
      break;
    }
    case RT_TOP:
    {
      trc.moveLeft(x - trc.width() / 2);
      trc.moveBottom(y - offset);
      break;
    }
    case RT_TOP_RIGHT:
    {
      int offset2 = 0.5 * sqrt(POW2(offset) + POW2(offset));
      trc.moveLeft(x + offset2);
      trc.moveBottom(y - offset2);
      break;
    }
    case RT_TOP_LEFT:
    {
      int offset2 = 0.5 * sqrt(POW2(offset) + POW2(offset));
      trc.moveRight(x - offset2);
      trc.moveBottom(y - offset2);
      break;
    }
    case RT_BOTTOM_RIGHT:
    {
      int offset2 = 0.5 * sqrt(POW2(offset) + POW2(offset));
      trc.moveLeft(x + offset2);
      trc.moveTop(y + offset2);
      break;
    }
    case RT_BOTTOM_LEFT:
    {
      int offset2 = 0.5 * sqrt(POW2(offset) + POW2(offset));
      trc.moveRight(x - offset2);
      trc.moveTop(y + offset2);
      break;
    }
    case RT_BOTTOM:
    {
      trc.moveLeft(x - trc.width() / 2);
      trc.moveTop(y + offset);
      break;
    }
    case RT_RIGHT:
    {
      trc.moveLeft(x + offset);
      trc.moveTop(y - trc.height() / 2);
      break;
    }
    case RT_LEFT:
    {
      trc.moveRight(x - offset);
      trc.moveTop(y - trc.height() / 2);
      break;
    }
  }
  trc.adjust(-1, -1 ,1 ,1);

  if (render)
  {
    drawText(trc, Qt::AlignCenter, text);
  }
  //drawRect(trc);

  return trc;
}



