#include "cscanrender.h"
#include "omp.h"
#include "skcore.h"

CScanRender scanRender;

//////////////////////////////
CScanRender::CScanRender(void)
//////////////////////////////
{
  bBilinear = false;
  m_opacity = 1.f;
}

/////////////////////////////////////////////////
void CScanRender::enableBillinearInt(bool enable)
/////////////////////////////////////////////////
{
  bBilinear = enable;
}

//////////////////////////////////
bool CScanRender::isBillinearInt()
//////////////////////////////////
{
  return(bBilinear);
}

///////////////////////////////////////////////
void CScanRender::resetScanPoly(int sx, int sy)
///////////////////////////////////////////////
{
  plMinY =  999999;
  plMaxY = -999999;

  if (sy >= MAX_BK_SCANLINES)
  {
    qDebug("CScanRender::resetScanPoly fail!");
    return;
  }

  m_sx = sx;
  m_sy = sy;
}


/////////////////////////////////////
static inline int float2int(double d)
/////////////////////////////////////
{
   union Cast
   {
      double d;
      long l;
    };
   volatile Cast c;
   c.d = d + 6755399441055744.0;
   return c.l;
}


//////////////////////////////////////////////////////////
void CScanRender::scanLine(int x1, int y1, int x2, int y2)
//////////////////////////////////////////////////////////
{
  int side;

  if (y1 > y2)
  {
    qSwap(x1, x2);
    qSwap(y1, y2);
    side = 0;
  }
  else
  {
    side = 1;
  }

  if (y2 < 0)
  {
    return; // offscreen
  }

  if (y1 >= m_sy)
  {
    return; // offscreen
  }

  float dy = (float)(y2 - y1);

  if (dy == 0) // hor. line
  {
    return;
  }

  float dx = (float)(x2 - x1) / dy;
  float x = x1;
  int   y;

  if (y2 >= m_sy)
  {
    y2 = m_sy - 1;
  }

  if (y1 < 0)
  { // partialy off screen
    float m = (float) -y1;

    x += dx * m;
    y1 = 0;
  }

  int minY = qMin(y1, y2);
  int maxY = qMax(y1, y2);

  if (minY < plMinY)
    plMinY = minY;
  if (maxY > plMaxY)
    plMaxY = maxY;

#define SCAN_FIX_PT 1

#if SCAN_FIX_PT

#define FP 16

  int fx = (int)(x * (float)(1 << FP));
  int fdx = (int)(dx * (float)(1 << FP));

  for (y = y1; y <= y2; y++)
  {
    scLR[y].scan[side] = fx >> FP;
    fx += fdx;
  }

#else

  for (y = y1; y <= y2; y++)
  {
    if (side == 1)
    { // side left
      scLR[y].scan[0] = float2int(x);
    }
    else
    { // side right
      scLR[y].scan[1] = float2int(x);
    }
    x += dx;
  }

#endif
}


//////////////////////////////////////////////////////////////////////////////////////////////////
void CScanRender::scanLine(int x1, int y1, int x2, int y2, float u1, float v1, float u2, float v2)
//////////////////////////////////////////////////////////////////////////////////////////////////
{
  int side;

  if (y1 > y2)
  {
    qSwap(x1, x2);
    qSwap(y1, y2);
    qSwap(u1, u2);
    qSwap(v1, v2);
    side = 0;
  }
  else
  {
    side = 1;
  }

  if (y2 < 0)
    return; // offscreen
  if (y1 >= m_sy)
    return; // offscreen

  float dy = (float)(y2 - y1);

  if (dy == 0) // hor. line
    return;//dy = 1;

  float dx = (float)(x2 - x1) / dy;
  float x = x1;
  int   y;

  if (y2 >=  m_sy)
    y2 =  m_sy - 1;

  float duv[2];
  float uv[2] = {u1, v1};

  duv[0] = (u2 - u1) / dy;
  duv[1] = (v2 - v1) / dy;

  if (y1 < 0)
  { // partialy off screen
    float m = (float) -y1;

    uv[0] += duv[0] * m;
    uv[1] += duv[1] * m;

    x += dx * m;
    y1 = 0;
  }

  int minY = qMin(y1, y2);
  int maxY = qMax(y1, y2);

  if (minY < plMinY)
    plMinY = minY;
  if (maxY > plMaxY)
    plMaxY = maxY;

  for (y = y1; y <= y2; y++)
  {
    scLR[y].scan[side] = float2int(x);
    scLR[y].uv[side][0] = uv[0];
    scLR[y].uv[side][1] = uv[1];

    x += dx;

    uv[0] += duv[0];
    uv[1] += duv[1];
  }
}


////////////////////////////////////////////////////////
void CScanRender::renderPolygon(QColor col, QImage *dst)
////////////////////////////////////////////////////////
{
  quint32   c = col.rgb();
  quint32  *bits = (quint32 *)dst->bits();
  int       dw = dst->width();
  bkScan_t *scan = scLR;

  for (int y = plMinY; y <= plMaxY; y++)
  {
    int px1 = scan[y].scan[0];
    int px2 = scan[y].scan[1];

    if (px1 > px2)
    {
      qSwap(px1, px2);
    }

    if (px1 < 0)
    {
      px1 = 0;
      if (px2 < 0)
      {
        continue;
      }
    }

    if (px2 >= m_sx)
    {
      if (px1 >= m_sx)
      {
        continue;
      }
      px2 = m_sx - 1;
    }

    quint32 *pDst = bits + (y * dw) + px1;

    for (int x = px1; x < px2; x++)
    {
      *pDst = c;
      pDst++;
    }
  }
}


/////////////////////////////////////////////////////////////
void CScanRender::renderPolygonAlpha(QColor col, QImage *dst)
/////////////////////////////////////////////////////////////
{
  quint32   c = col.rgba();
  quint32  *bits = (quint32 *)dst->bits();
  int       dw = dst->width();
  bkScan_t *scan = scLR;
  float     a = qAlpha(c) / 256.0f;

  for (int y = plMinY; y <= plMaxY; y++)
  {
    int px1 = scan[y].scan[0];
    int px2 = scan[y].scan[1];

    if (px1 > px2)
    {
      qSwap(px1, px2);
    }

    if (px1 < 0)
    {
      px1 = 0;
    }

    if (px2 >= m_sx)
    {
      px2 = m_sx - 1;
    }

    quint32 *pDst = bits + (y * dw) + px1;
    for (int x = px1; x < px2; x++)
    {
      QRgb rgbd = *pDst;

      *pDst = qRgb(LERP(a, qRed(rgbd), qRed(c)),
                   LERP(a, qGreen(rgbd), qGreen(c)),
                   LERP(a, qBlue(rgbd), qBlue(c))
                  );
      pDst++;
    }
  }
}

void CScanRender::setOpacity(float opacity)
{
  m_opacity = opacity;
}

#define number2int(i,d)  { __asm fld d;   __asm fistp i; }

/////////////////////////////////////////////////////////
void CScanRender::renderPolygon(QImage *dst, QImage *src)
/////////////////////////////////////////////////////////
{
  if (bBilinear)
    renderPolygonBI(dst, src);
  else
    renderPolygonNI(dst, src);
}

///////////////////////////////////////////////////////////
void CScanRender::renderPolygonNI(QImage *dst, QImage *src)
///////////////////////////////////////////////////////////
{
  int w = dst->width();
  int sw = src->width();
  double tsx = src->width() - 1;
  double tsy = src->height() - 1;
  const quint32 *bitsSrc = (quint32 *)src->constBits();
  quint32 *bitsDst = (quint32 *)dst->bits();
  bkScan_t *scan = scLR;
  bool bw = src->format() == QImage::Format_Indexed8 || src->format() == QImage::Format_Grayscale8;

  for (int y = plMinY; y <= plMaxY; y++)
  {
    if (scan[y].scan[0] > scan[y].scan[1])
    {
      qSwap(scan[y].scan[0], scan[y].scan[1]);
      qSwap(scan[y].uv[0][0], scan[y].uv[1][0]);
      qSwap(scan[y].uv[0][1], scan[y].uv[1][1]);
    }

    int px1 = scan[y].scan[0];
    int px2 = scan[y].scan[1];

    double dx = px2 - px1;
    if (dx == 0)
      continue;

    double duv[2];
    double uv[2];

    duv[0] = (double)(scan[y].uv[1][0] - scan[y].uv[0][0]) / dx;
    duv[1] = (double)(scan[y].uv[1][1] - scan[y].uv[0][1]) / dx;

    uv[0] = scan[y].uv[0][0];
    uv[1] = scan[y].uv[0][1];

    if (px1 < 0)
    {
      double m = (double)-px1;

      px1 = 0;
      uv[0] += duv[0] * m;
      uv[1] += duv[1] * m;
    }

    if (px2 >= w)
      px2 = w - 1;

    uv[0] *= tsx;
    uv[1] *= tsy;

    duv[0] *= tsx;
    duv[1] *= tsy;

    quint32 *pDst = bitsDst + (y * w) + px1;

    if (bw)
    {
      for (int x = px1; x < px2; x++)
      {
        const uchar *pSrc = (uchar *)bitsSrc + ((int)(uv[0]) + (int)(uv[1]) * sw);
        *pDst = qRgb(*pSrc, *pSrc, *pSrc);
        pDst++;

        uv[0] += duv[0];
        uv[1] += duv[1];
      }
    }
    else
    {
      for (int x = px1; x < px2; x++)
      {
        const quint32 *pSrc = bitsSrc + ((int)(uv[0]) + (int)(uv[1]) * sw);
        *pDst = *pSrc;
        pDst++;

        uv[0] += duv[0];
        uv[1] += duv[1];
      }
    }
  }
}


///////////////////////////////////////////////////////////
void CScanRender::renderPolygonBI(QImage *dst, QImage *src)
///////////////////////////////////////////////////////////
{
  int w = dst->width();
  int sw = src->width();
  int sh = src->height();
  double tsx = src->width() - 1;
  double tsy = src->height() - 1;
  const quint32 *bitsSrc = (quint32 *)src->constBits();
  const uchar *bitsSrc8 = (uchar *)src->constBits();
  quint32 *bitsDst = (quint32 *)dst->bits();
  bkScan_t *scan = scLR;
  bool bw = src->format() == QImage::Format_Indexed8 || src->format() == QImage::Format_Grayscale8;

  for (int y = plMinY; y <= plMaxY; y++)
  {
    if (scan[y].scan[0] > scan[y].scan[1])
    {
      qSwap(scan[y].scan[0], scan[y].scan[1]);
      qSwap(scan[y].uv[0][0], scan[y].uv[1][0]);
      qSwap(scan[y].uv[0][1], scan[y].uv[1][1]);
    }

    int px1 = scan[y].scan[0];
    int px2 = scan[y].scan[1];

    double dx = px2 - px1;
    if (dx == 0)
      continue;

    double duv[2];
    double uv[2];

    duv[0] = (double)(scan[y].uv[1][0] - scan[y].uv[0][0]) / dx;
    duv[1] = (double)(scan[y].uv[1][1] - scan[y].uv[0][1]) / dx;

    uv[0] = scan[y].uv[0][0];
    uv[1] = scan[y].uv[0][1];

    if (px1 < 0)
    {
      double m = (float)-px1;

      px1 = 0;
      uv[0] += duv[0] * m;
      uv[1] += duv[1] * m;
    }

    if (px2 >= w)
      px2 = w - 1;

    uv[0] *= tsx;
    uv[1] *= tsy;

    duv[0] *= tsx;
    duv[1] *= tsy;

    int size = sw * sh;

    quint32 *pDst = bitsDst + (y * w) + px1;
    if (bw)
    {
      for (int x = px1; x < px2; x++)
      {
        double x_diff = uv[0] - static_cast<int>(uv[0]);
        double y_diff = uv[1] - static_cast<int>(uv[1]);
        double x_1diff = 1 - x_diff;
        double y_1diff = 1 - y_diff;

        int index = ((int)uv[0] + ((int)uv[1] * sw));

        uchar a = bitsSrc8[index];
        uchar b = bitsSrc8[(index + 1) % size];
        uchar c = bitsSrc8[(index + sw) % size];
        uchar d = bitsSrc8[(index + sw + 1) % size];

        int val = (a&0xff)*(x_1diff)*(y_1diff) + (b&0xff)*(x_diff)*(y_1diff) +
                  (c&0xff)*(y_diff)*(x_1diff)   + (d&0xff)*(x_diff*y_diff);

        // ????: pretypovani to zrychly
        *pDst = 0xff000000 |
                ((((int)val)<<16)&0xff0000) |
                ((((int)val)<<8)&0xff00) |
                ((int)val) ;
        pDst++;

        uv[0] += duv[0];
        uv[1] += duv[1];
      }
    }
    else
    {
      for (int x = px1; x < px2; x++)
      {
        double x_diff = uv[0] - static_cast<int>(uv[0]);
        double y_diff = uv[1] - static_cast<int>(uv[1]);
        double x_1diff = 1 - x_diff;
        double y_1diff = 1 - y_diff;

        int index = ((int)uv[0] + ((int)uv[1] * sw));

        quint32 a = bitsSrc[index];
        quint32 b = bitsSrc[(index + 1) % size];
        quint32 c = bitsSrc[(index + sw) % size];
        quint32 d = bitsSrc[(index + sw + 1) % size];

        double xy1 = x_1diff * y_1diff;
        double xy2 = x_diff * y_1diff;
        double xy = x_diff * y_diff;
        double yx1 = y_diff * x_1diff;

        // blue element
        int blue = (a&0xff)*(xy1) + (b&0xff)*(xy2) +
                   (c&0xff)*(yx1)  + (d&0xff)*(xy);

        // green element
        int green = ((a>>8)&0xff)*(xy1) + ((b>>8)&0xff)*(xy2) +
                    ((c>>8)&0xff)*(yx1)  + ((d>>8)&0xff)*(xy);

        // red element
        int red = ((a>>16)&0xff)*(xy1) + ((b>>16)&0xff)*(xy2) +
                  ((c>>16)&0xff)*(yx1)  + ((d>>16)&0xff)*(xy);

        // ????: pretypovani to zrychly

        *pDst = 0xff000000 |
                ((((int)red)<<16)&0xff0000) |
                ((((int)green)<<8)&0xff00) |
                ((int)blue) ;
        pDst++;

        uv[0] += duv[0];
        uv[1] += duv[1];
      }
    }
  }
}

void CScanRender::renderPolygonAlpha(QImage *dst, QImage *src)
{
  if (bBilinear)
    renderPolygonAlphaBI(dst, src);
  else
    renderPolygonAlphaNI(dst, src);
}

void CScanRender::renderPolygonAlphaBI(QImage *dst, QImage *src)
{
  int w = dst->width();
  int sw = src->width();
  int sh = src->height();
  float tsx = src->width() - 1;
  float tsy = src->height() - 1;
  const quint32 *bitsSrc = (quint32 *)src->constBits();
  //const uchar *bitsSrc8 = (uchar *)src->constBits();
  quint32 *bitsDst = (quint32 *)dst->bits();
  bkScan_t *scan = scLR;
  bool bw = src->format() == QImage::Format_Indexed8;

  #pragma omp parallel for shared(bitsDst, bitsSrc, scan, tsx, tsy, w, sw)
  for (int y = plMinY; y <= plMaxY; y++)
  {
    if (scan[y].scan[0] > scan[y].scan[1])
    {
      qSwap(scan[y].scan[0], scan[y].scan[1]);
      qSwap(scan[y].uv[0][0], scan[y].uv[1][0]);
      qSwap(scan[y].uv[0][1], scan[y].uv[1][1]);
    }

    int px1 = scan[y].scan[0];
    int px2 = scan[y].scan[1];

    float dx = px2 - px1;
    if (dx == 0)
      continue;

    float duv[2];
    float uv[2];

    duv[0] = (float)(scan[y].uv[1][0] - scan[y].uv[0][0]) / dx;
    duv[1] = (float)(scan[y].uv[1][1] - scan[y].uv[0][1]) / dx;

    uv[0] = scan[y].uv[0][0];
    uv[1] = scan[y].uv[0][1];

    if (px1 < 0)
    {
      float m = (float)-px1;

      px1 = 0;
      uv[0] += duv[0] * m;
      uv[1] += duv[1] * m;
    }

    if (px2 >= w)
      px2 = w - 1;

    uv[0] *= tsx;
    uv[1] *= tsy;

    duv[0] *= tsx;
    duv[1] *= tsy;

    int size = sw * sh;

    quint32 *pDst = bitsDst + (y * w) + px1;
    if (bw)
    {
      /*
      for (int x = px1; x <= px2; x++)
      {
        float x_diff = uv[0] - static_cast<int>(uv[0]);
        float y_diff = uv[1] - static_cast<int>(uv[1]);
        float x_1diff = 1 - x_diff;
        float y_1diff = 1 - y_diff;

        int index = ((int)uv[0] + ((int)uv[1] * sw));

        uchar a = bitsSrc8[index];
        uchar b = bitsSrc8[(index + 1) % size];
        uchar c = bitsSrc8[(index + sw) % size];
        uchar d = bitsSrc8[(index + sw + 1) % size];

        int val = (a&0xff)*(x_1diff)*(y_1diff) + (b&0xff)*(x_diff)*(y_1diff) +
                  (c&0xff)*(y_diff)*(x_1diff)   + (d&0xff)*(x_diff*y_diff);


        *pDst = 0xff000000 |
                ((((int)val)<<16)&0xff0000) |
                ((((int)val)<<8)&0xff00) |
                ((int)val) ;
        pDst++;

        uv[0] += duv[0];
        uv[1] += duv[1];
      }
      */
    }
    else
    {
      for (int x = px1; x < px2; x++)
      {
        float x_diff = uv[0] - static_cast<int>(uv[0]);
        float y_diff = uv[1] - static_cast<int>(uv[1]);
        float x_1diff = 1 - x_diff;
        float y_1diff = 1 - y_diff;

        int index = ((int)uv[0] + ((int)uv[1] * sw));

        quint32 a = bitsSrc[index];
        quint32 b = bitsSrc[(index + 1) % size];
        quint32 c = bitsSrc[(index + sw) % size];
        quint32 d = bitsSrc[(index + sw + 1) % size];

        float x1y1 = x_1diff * y_1diff;
        float xy = x_diff * y_diff;
        float x1y = y_diff * x_1diff;
        float xy1 = x_diff *y_1diff;

        float alpha = 0.00390625f * (((a>>24)&0xff)*(x1y1) + ((b>>24)&0xff)*(xy1) +
                     ((c>>24)&0xff)*(x1y) + ((d>>24)&0xff)*(xy)) * m_opacity;

        if (alpha > 0.00390625f) // > 1 / 256.
        {
          // blue element
          int blue = (a&0xff)*(x1y1) + (b&0xff)* (xy1) +
                     (c&0xff)*(x1y)  + (d&0xff)*(xy);

          // green element
          int green = ((a>>8)&0xff)*(x1y1) + ((b>>8)&0xff)*(xy1) +
                      ((c>>8)&0xff)*(x1y)  + ((d>>8)&0xff)*(xy);

          // red element
          int red = ((a>>16)&0xff)*(x1y1) + ((b>>16)&0xff)*(xy1) +
                    ((c>>16)&0xff)*(x1y)  + ((d>>16)&0xff)*(xy);

          *pDst = qRgb(LERP(alpha, qRed(*pDst),  red),
                       LERP(alpha, qGreen(*pDst), green),
                       LERP(alpha, qBlue(*pDst), blue)
                       );
        }

        pDst++;

        uv[0] += duv[0];
        uv[1] += duv[1];
      }
    }
  }
}


////////////////////////////////////////////////////////////////
void CScanRender::renderPolygonAlphaNI(QImage *dst, QImage *src)
////////////////////////////////////////////////////////////////
{
  int w = dst->width();
  int sw = src->width();
  float tsx = src->width() - 1;
  float tsy = src->height() - 1;
  const quint32 *bitsSrc = (quint32 *)src->constBits();
  quint32 *bitsDst = (quint32 *)dst->bits();
  bkScan_t *scan = scLR;

  #pragma omp parallel for shared(bitsDst, bitsSrc, scan, tsx, tsy, w, sw)
  for (int y = plMinY; y <= plMaxY; y++)
  {
    if (scan[y].scan[0] > scan[y].scan[1])
    {
      qSwap(scan[y].scan[0], scan[y].scan[1]);
      qSwap(scan[y].uv[0][0], scan[y].uv[1][0]);
      qSwap(scan[y].uv[0][1], scan[y].uv[1][1]);
    }

    int px1 = scan[y].scan[0];
    int px2 = scan[y].scan[1];

    float dx = px2 - px1;
    if (dx == 0)
      continue;

    float duv[2];
    float uv[2];

    duv[0] = (float)(scan[y].uv[1][0] - scan[y].uv[0][0]) / dx;
    duv[1] = (float)(scan[y].uv[1][1] - scan[y].uv[0][1]) / dx;

    uv[0] = scan[y].uv[0][0];
    uv[1] = scan[y].uv[0][1];

    if (px1 < 0)
    {
      float m = (float)-px1;

      px1 = 0;
      uv[0] += duv[0] * m;
      uv[1] += duv[1] * m;
    }

    if (px2 >= w)
      px2 = w - 1;

    quint32 *pDst = bitsDst + (y * w) + px1;

    uv[0] *= tsx;
    uv[1] *= tsy;

    duv[0] *= tsx;
    duv[1] *= tsy;

    for (int x = px1; x < px2; x++)
    {
      const quint32 *pSrc = bitsSrc + ((int)(uv[0])) + ((int)(uv[1]) * sw);
      QRgb rgbs = *pSrc;
      QRgb rgbd = *pDst;
      float a = qAlpha(*pSrc) * 0.00390625f * m_opacity;

      if (a > 0.00390625f)
      {
        *pDst = qRgb(LERP(a, qRed(rgbd), qRed(rgbs)),
                     LERP(a, qGreen(rgbd), qGreen(rgbs)),
                     LERP(a, qBlue(rgbd), qBlue(rgbs))
                     );
      }
      pDst++;

      uv[0] += duv[0];
      uv[1] += duv[1];
    }
  }
}

