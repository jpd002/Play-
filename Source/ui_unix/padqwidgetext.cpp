#include "padqwidgetext.h"
#include <QKeyEvent>
#include <QLayout>

PadQWidgetExt::PadQWidgetExt(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);


    m_button = new QButtonExt(this);
    mainLayout->addWidget(m_button) ;

    m_label = new QLabel("Not Mapped", this);
    m_label->setFrameShape(QFrame::Box);
    m_label->setFrameShadow(QFrame::Raised);
    m_label->setLineWidth(2);
    m_label->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_label);

    setLayout(mainLayout);

    connect(m_button, SIGNAL(pressed()), this, SLOT(clicked()));
    connect(m_button, SIGNAL(focusout()), this, SLOT(focusout()));

    connect(this, SIGNAL(objectNameChanged(QString)), this, SLOT(setButtonText(QString)));
}

void PadQWidgetExt::clicked(){
    m_label->setStyleSheet("background: rgb(210, 0, 0);");
    connect(m_button, SIGNAL(keyDown(QKeyEvent*)),this, SLOT(keyDown(QKeyEvent*)));
}

void PadQWidgetExt::focusout()
{
    m_label->setStyleSheet("");
    m_key = QKeySequence(m_label->text());
    disconnect(m_button, SIGNAL(keyDown(QKeyEvent*)), this, SLOT(keyDown(QKeyEvent*)));
}

void PadQWidgetExt::keyDown(QKeyEvent* ev)
{
    Qt::Key ev_key = (Qt::Key)ev->key();
    if (ev_key == Qt::Key_Escape){
        QKeySequence seq;
        if (!m_key.isEmpty())
        {
            seq = m_key;
        }
        else if (!property("DefaultKey").isNull())
        {
            seq = QKeySequence(property("DefaultKey").toString());
        }
        m_label->setText(seq.toString());
        focusout();
    } else {
        m_label->setText(QKeySequence(ev_key).toString());
    }
}

void PadQWidgetExt::setkey(QKeySequence new_key)
{
    m_key = new_key;
}

void PadQWidgetExt::setLabelText(QString text)
{
    m_label->setText(text);
}

void PadQWidgetExt::setButtonText(QString text)
{
    m_button->setText(text.replace("_2","").replace("_","-"));
}

QKeySequence PadQWidgetExt::key()
{
    if (!m_key.isEmpty())
    {
        return m_key;
    }
    else if (!property("DefaultKey").isNull())
    {
        return QKeySequence(property("DefaultKey").toString());
    }
    return QKeySequence();
}

PadQWidgetExt::ControllerType PadQWidgetExt::ControlType()
{
    return m_type;
}

int PadQWidgetExt::ControlTypeInt()
{
    return m_type;
}

void PadQWidgetExt::setControlType(int type)
{
    if (m_type == 0 || m_type ==1)
    {
        m_type = static_cast<ControllerType>(type);
    } else {
        m_type = ControllerType::Keyboard;
    }
}

int PadQWidgetExt::bindingIndex()
{
    return property("BindingIndex").toInt();
}

void PadQWidgetExt::restoreDefault()
{
    m_key = QKeySequence(property("DefaultKey").toString());
    if (!m_key.isEmpty())
    {
        m_label->setText(m_key.toString());
    } else {
        m_label->setText("Not Mapped");
    }
}

QButtonExt::QButtonExt(QWidget *parent)
    : QPushButton(parent)
{
}

QButtonExt::~QButtonExt(){}

void QButtonExt::focusOutEvent(QFocusEvent * ev)
{
    emit focusout();
}

void QButtonExt::keyPressEvent(QKeyEvent* ev)
{
    if (hasFocus())
    {
         emit keyDown(ev);
    }
}
void QButtonExt::keyReleaseEvent(QKeyEvent* ev)
{
    emit focusout();
}
