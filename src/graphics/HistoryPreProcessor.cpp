#include "HistoryPreProcessor.h"
#include "Bitmap.h"
#include "Filterfill.h"

#include <iostream>

using namespace std;

namespace avg {
    
HistoryPreProcessor::HistoryPreProcessor(IntPoint dimensions, unsigned int UpdateInterval)
    : m_FrameCounter(0),
      m_UpdateInterval(UpdateInterval),
      m_HistoryInitialized(1)
{
    m_pHistoryBmp = BitmapPtr(new Bitmap(dimensions, I16));
}

HistoryPreProcessor::~HistoryPreProcessor()
{
}
void HistoryPreProcessor::setInterval(unsigned int UpdateInterval)
{
    m_FrameCounter = 0;
    m_UpdateInterval = UpdateInterval;
}

void HistoryPreProcessor::reset()
{
    m_HistoryInitialized = 100;
}

void HistoryPreProcessor::updateHistory(BitmapPtr new_img)
{
    assert(new_img->getSize() == m_pHistoryBmp->getSize());
    //else
    if (m_HistoryInitialized <= 0) {
        if (m_FrameCounter < m_UpdateInterval-1||(m_HistoryInitialized<0)){
            m_FrameCounter++;
            return;
        }
        m_FrameCounter = 0;
        const unsigned char * pSrc = new_img->getPixels();
        unsigned short * pDest = (unsigned short*)(m_pHistoryBmp->getPixels());
        int DestStride = m_pHistoryBmp->getStride()/m_pHistoryBmp->getBytesPerPixel();
        IntPoint Size = m_pHistoryBmp->getSize();
        for (int y=0; y<Size.y; y++) {
            const unsigned char * pSrcPixel = pSrc;
            unsigned short * pDestPixel = pDest;
            for (int x=0; x<Size.x; x++) {
                int t = 255*int(*pDestPixel);
                *pDestPixel = (t)/256 + *pSrcPixel;
                pDestPixel++;
                pSrcPixel++;
            }
            pDest += DestStride;
            pSrc += new_img->getStride();
        }
        m_HistoryInitialized = (m_HistoryInitialized<0)?m_HistoryInitialized++:0;
    } else {
        m_pHistoryBmp->copyPixels(*new_img);
        m_HistoryInitialized *= -1;
    }
}

void HistoryPreProcessor::applyInPlace(BitmapPtr img){
    updateHistory(img);
    unsigned short * pSrc = (unsigned short*)m_pHistoryBmp->getPixels();
    int SrcStride = m_pHistoryBmp->getStride()/m_pHistoryBmp->getBytesPerPixel();
    unsigned char * pDest = img->getPixels();
    IntPoint Size = img->getSize();
    for (int y=0; y<Size.y; y++) {
        const unsigned short * pSrcPixel = pSrc;
        unsigned char * pDestPixel = pDest;
        for (int x=0; x<Size.x; x++) {
            //*pDestPixel = (unsigned char)abs((int)*pDestPixel - (int)(*pSrcPixel)/256);
            *pDestPixel = (*pDestPixel)>(*pSrcPixel/256)?*pDestPixel:0;
            pDestPixel++;
            pSrcPixel++;
        }
        pDest += img->getStride();
        pSrc += SrcStride;
    }
}
}
