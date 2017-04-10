#pragma once

#include <QLabel>

// MIT - Yash : Speedovation.com [ Picked from internet and modified. if owner needs to add or change license do let me know.]
// as provided by https://wiki.qt.io/Elided_Label

class ElidedLabel : public QLabel 
{
    Q_OBJECT
public:
    ElidedLabel(QWidget *parent=Q_NULLPTR, Qt::WindowFlags f=Qt::WindowFlags());
    ElidedLabel(const QString& txt, QWidget *parent=Q_NULLPTR, Qt::WindowFlags f=Qt::WindowFlags());
    ElidedLabel(const QString& txt, Qt::TextElideMode elideMode=Qt::ElideRight, QWidget *parent=Q_NULLPTR, Qt::WindowFlags f=Qt::WindowFlags());
    // Set the elide mode used for displaying text.
    void setElideMode(Qt::TextElideMode elideMode) {
        elideMode_ = elideMode;
        updateGeometry();
    }
    // Get the elide mode currently used to display text.
    Qt::TextElideMode elideMode() const { return elideMode_; }
    // QLabel overrides
    void setText(const QString &);

protected: // QLabel overrides
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);
    // Cache the elided text so as to not recompute it every paint event
    void cacheElidedText(int w);

private:
    Qt::TextElideMode elideMode_;
    QString cachedElidedText;
};
