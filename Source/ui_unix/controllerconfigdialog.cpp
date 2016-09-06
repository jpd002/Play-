#include "controllerconfigdialog.h"
#include "ui_controllerconfigdialog.h"
#include "ControllerInfo.h"

#include <QRegularExpression>
#include <QLabel>
#include <QFile>

#include "AppConfig.h"

ControllerConfigDialog::ControllerConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ControllerConfigDialog)
{
    ui->setupUi(this);

    Load(1, ui->tab);
    ui->tabWidget->removeTab(1);
    //Load(2, ui->tab_2);
}

ControllerConfigDialog::~ControllerConfigDialog()
{
    delete ui;
}

void ControllerConfigDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if(button == ui->buttonBox->button(QDialogButtonBox::Ok))
    {
        Save(1, ui->tab);
        //Save(2, ui->tab_2);
    }
    else if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
    {
        Save(ui->tabWidget->currentIndex()+1, ui->tabWidget->currentWidget());
    }
    else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults))
    {
        switch (ui->tabWidget->currentIndex())
        {
        case 0:
            foreach (PadQWidgetExt* child, ui->tab->findChildren<PadQWidgetExt *>()) child->restoreDefault();
            break;
        case 1:
            foreach (PadQWidgetExt* child, ui->tab_2->findChildren<PadQWidgetExt *>()) child->restoreDefault();
            break;
        }
    }
}

void ControllerConfigDialog::Save(int index, QWidget* tab)
{

    QMap<QString, PadQWidgetExt*> map;
    foreach (PadQWidgetExt* child, tab->findChildren<PadQWidgetExt *>())
    {
        //Sorted for a more predictable order
        QString oname (child->objectName());
        map.insert(oname, child);

    }
    QString path(CAppConfig::GetBasePath().c_str());
    QString filename="keymaps_%1.xml";
    filename = filename.arg(QString::number(index));
    QFile file(path +"/"+ filename);
    file.open(QIODevice::WriteOnly);

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();

    xmlWriter.writeStartElement("KeyMaps");
    for (auto widget = map.begin(); widget != map.end(); ++widget)
    {
        PadQWidgetExt* child = widget.value();
        if (!child->key().isEmpty())
        {
            QKeySequence key = child->key();
            QString oname = child->objectName();
            int bindingindex = child->bindingIndex();

            xmlWriter.writeStartElement("Key");
            xmlWriter.writeTextElement("Name", oname);
            xmlWriter.writeTextElement("BindingIndex", QString::number(bindingindex) );
            xmlWriter.writeTextElement("Type", QString::number(child->ControlType()));
            xmlWriter.writeTextElement("Value", QString::number(key[0]));
            xmlWriter.writeEndElement();

        }
    }
    xmlWriter.writeEndElement();
    file.close();
}

void ControllerConfigDialog::Load(int index, QWidget* tab)
{
    QXmlStreamReader Rxml;
    QString path(CAppConfig::GetBasePath().c_str());
    QString filename="keymaps_%1.xml";
    filename = filename.arg(QString::number(index));
    QFile file(path +"/"+ filename);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        foreach (PadQWidgetExt* child, tab->findChildren<PadQWidgetExt *>())
            child->restoreDefault();
        return;
    }

    Rxml.setDevice(&file);
    Rxml.readNext();

    while(!Rxml.atEnd())
    {
        if(Rxml.isStartElement())
        {
            if(Rxml.name() == "KeyMaps")
            {
                Rxml.readNext();
            }
            else if(Rxml.name() == "Key")
            {
                PadQWidgetExt* child = nullptr;
                while(!Rxml.atEnd())
                {
                    if(Rxml.isEndElement())
                    {
                        Rxml.readNext();
                        break;
                    }
                    else if(Rxml.isCharacters())
                    {
                        Rxml.readNext();
                    }
                    else if(Rxml.isStartElement())
                    {
                        if(Rxml.name() == "Name")
                        {
                            QString value = ReadElementValue(Rxml);
                            QRegularExpression re = QRegularExpression(QString("%1").arg(value.toStdString().c_str()).toStdString().c_str());
                            child = tab->findChildren<PadQWidgetExt*>(re).first();
                            if (child == nullptr) break;

                        }
                        else if(Rxml.name() == "BindingIndex")
                        {
                            if (child == nullptr) break;
                            int value = ReadElementValue(Rxml).toInt();
                            int bindingindex = child->bindingIndex();
                            if (value != bindingindex) break;
                        }
                        else if(Rxml.name() == "Type")
                        {
                            ReadElementValue(Rxml);
                        }
                        else if(Rxml.name() == "Value")
                        {
                            if (child == nullptr) break;
                            QString value = ReadElementValue(Rxml);
                            QKeySequence key(value.toInt());
                            child->setLabelText(key.toString());
                            child->setkey(key);

                        }
                        Rxml.readNext();
                    }
                    else
                    {
                        Rxml.readNext();
                    }
                }
            }
        }
        else
        {
            Rxml.readNext();
        }
    }
    file.close();

    if (Rxml.hasError())
    {
        fprintf(stderr, "Error: Failed to parse file %s: %s\n", filename.toStdString().c_str() ,file.errorString().toStdString().c_str());
    }
    else if (file.error() != QFile::NoError)
    {
        fprintf(stderr, "Error: Cannot read file %s: %s\n", filename.toStdString().c_str() ,file.errorString().toStdString().c_str());
    }
}

QString ControllerConfigDialog::ReadElementValue(QXmlStreamReader &Rxml)
{
    while(!Rxml.atEnd())
    {
        if(Rxml.isEndElement())
        {
            Rxml.readNext();
            break;
        }
        else if(Rxml.isStartElement())
        {
            QString value = Rxml.readElementText();
            return value;
            break;
        }
        else if(Rxml.isCharacters())
        {
            Rxml.readNext();
        }
        else
        {
            Rxml.readNext();
        }
    }
    return "";
}

CPH_HidUnix::BindingPtr *ControllerConfigDialog::GetBinding(int pad)
{
    uint32 m_axis_bindings[4] = {0};
    CPH_HidUnix::BindingPtr*			m_bindings = new CPH_HidUnix::BindingPtr[PS2::CControllerInfo::MAX_BUTTONS];
    QXmlStreamReader Rxml;
    QString path(CAppConfig::GetBasePath().c_str());
    QString filename="keymaps_%1.xml";
    filename = filename.arg(QString::number(pad));
    QFile file(path +"/"+ filename);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        return nullptr;
    }

    Rxml.setDevice(&file);
    Rxml.readNext();

    while(!Rxml.atEnd())
    {
        if(Rxml.isStartElement())
        {
            if(Rxml.name() == "KeyMaps")
            {
                Rxml.readNext();
            }
            else if(Rxml.name() == "Key")
            {
                QKeySequence key;
                int bindingindex = -1;
                int type = 0;
                while(!Rxml.atEnd())
                {
                    if(Rxml.isEndElement())
                    {
                        Rxml.readNext();
                        break;
                    }
                    else if(Rxml.isCharacters())
                    {
                        Rxml.readNext();
                    }
                    else if(Rxml.isStartElement())
                    {
                        if(Rxml.name() == "Name")
                        {
                            ReadElementValue(Rxml);
                        }
                        else if(Rxml.name() == "BindingIndex")
                        {
                            bindingindex = ReadElementValue(Rxml).toInt();
                        }
                        else if(Rxml.name() == "Type")
                        {
                            QString value = ReadElementValue(Rxml);
                            type = value.toInt();

                        }
                        else if(Rxml.name() == "Value")
                        {
                            QString value = ReadElementValue(Rxml);
                            key = QKeySequence(value.toInt());

                        }
                        Rxml.readNext();
                    }
                    else
                    {
                        Rxml.readNext();
                    }
                }
                if (bindingindex != -1 && !key.isEmpty())
                {
                    auto currentButtonId = static_cast<PS2::CControllerInfo::BUTTON>(bindingindex);
                    if(PS2::CControllerInfo::IsAxis(currentButtonId)) {
                        if (currentButtonId == PS2::CControllerInfo::ANALOG_LEFT_Y || currentButtonId == PS2::CControllerInfo::ANALOG_RIGHT_Y) {//Invert Up/Down
                            if (m_axis_bindings[bindingindex] == 0)
                            {
                                m_axis_bindings[bindingindex]= key[0];
                            } else {
                                m_bindings[bindingindex] = std::make_shared<CPH_HidUnix::CSimulatedAxisBinding>(key[0], m_axis_bindings[bindingindex], type);
                            }
                        } else {
                            if (m_axis_bindings[bindingindex] == 0)
                            {
                                m_axis_bindings[bindingindex]= key[0];
                            } else {
                                m_bindings[bindingindex] = std::make_shared<CPH_HidUnix::CSimulatedAxisBinding>(m_axis_bindings[bindingindex], key[0]);
                            }
                        }
                    } else {
                        m_bindings[bindingindex] = std::make_shared<CPH_HidUnix::CSimpleBinding>(key[0], type);
                    }
                }
            }
        }
        else
        {
            Rxml.readNext();
        }
    }
    file.close();

    if (Rxml.hasError())
    {
        fprintf(stderr, "Error: Failed to parse file %s: %s\n", filename.toStdString().c_str() ,file.errorString().toStdString().c_str());
    }
    else if (file.error() != QFile::NoError)
    {
        fprintf(stderr, "Error: Cannot read file %s: %s\n", filename.toStdString().c_str() ,file.errorString().toStdString().c_str());
    }
    return m_bindings;
}
