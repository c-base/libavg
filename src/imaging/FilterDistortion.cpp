#include "FilterDistortion.h"
#include "../graphics/Rect.h"
#include "../graphics/Point.h"
#include <boost/multi_array.hpp>
#include <math.h>

//Really ugly code. do not use...


namespace avg{
    FilterDistortion::FilterDistortion(IntPoint srcSize, double K1, double T):
        m_srcRect(0,0,srcSize.x,srcSize.y),
        m_trafo(m_srcRect, -K1, -T) //inverse of Distortion by reversing the sign.
    {
        //we use the same dimensions for both of src and dest and just crop...
        //for each pixel at (x,y) in the dest
        //m_pMap[x][y] contains a IntPoint that gives the coords in the src Bitmap 
        m_pMap = new pixel_map(boost::extents[m_srcRect.Height()][m_srcRect.Width()]);

        for(index y=0;y<srcSize.y;++y){
            for(index x=0;x<srcSize.x;++x){
                DPoint tmp = m_trafo.transform_point(DPoint(int(x),int(y)));
                IntPoint tmp2 = IntPoint(round(tmp.x),round(tmp.y));
                if(m_srcRect.Contains(tmp2)){
                    (*m_pMap)[y][x] = tmp2;
                }else{
                    (*m_pMap)[y][x] = IntPoint(0,0); //booooh
                }

            }
        }
    }
    BitmapPtr FilterDistortion::apply(BitmapPtr pBmpSource){
        
        BitmapPtr res = BitmapPtr(new Bitmap(*pBmpSource));
        unsigned char *p = res->getPixels();
        unsigned char *src = pBmpSource->getPixels();
        unsigned char *pLine = p;
        int destStride = res->getStride();
        int srcStride = pBmpSource->getStride();
        int w=m_srcRect.Width();
        int h=m_srcRect.Height();
        for(index y=m_srcRect.tl.y;y<h;++y){
            for(index x=m_srcRect.tl.x;x<w;++x){
                IntPoint sCoords = (*m_pMap)[y][x];
                *pLine = src[sCoords.x + srcStride*sCoords.y];
                pLine++;
            }
            p+=destStride;
            pLine = p;
        }
        return res;

    }
}
