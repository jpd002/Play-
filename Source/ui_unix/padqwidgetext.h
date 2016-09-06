#ifndef PADQWIDGETEXT_H
#define PADQWIDGETEXT_H

#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>


class QButtonExt : public QPushButton
{
    Q_OBJECT
public:
    explicit QButtonExt(QWidget *parent = Q_NULLPTR);
    ~QButtonExt();


protected:
    void focusOutEvent(QFocusEvent*) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent*) Q_DECL_OVERRIDE;

signals:
    void focusout();
    void keyDown(QKeyEvent*);
};

class PadQWidgetExt : public QWidget
{
    Q_OBJECT
public:
    explicit PadQWidgetExt(QWidget *parent = 0);
    enum ControllerType {
        Keyboard,
        GamePad
    };
    QKeySequence key();
    ControllerType ControlType();
    int bindingIndex();
    void setkey(QKeySequence);
    void setLabelText(QString);


    void setControlType(int);
    int ControlTypeInt();
    void restoreDefault();

private:
    ControllerType m_type = Keyboard;
    QKeySequence m_key;
    QString m_defaultKey;
    QButtonExt* m_button;
    QLabel* m_label;

private slots:
    void clicked();
    void keyDown(QKeyEvent*);
    void focusout();
    void setButtonText(QString);

};

#endif // PADQWIDGETEXT_H
