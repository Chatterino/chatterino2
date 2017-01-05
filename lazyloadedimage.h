#ifndef LAZYLOADEDIMAGE_H
#define LAZYLOADEDIMAGE_H

#include "QString"
#include "QImage"

class LazyLoadedImage
{
public:
    LazyLoadedImage(QString url);
    LazyLoadedImage(QImage* image);

    QImage* image() {
        return m_image;
    }

private:
    QImage* m_image = NULL;
};

#endif // LAZYLOADEDIMAGE_H
