#include "inputeventselectiondialog.h"
#include "ui_inputeventselectiondialog.h"

InputEventSelectionDialog::InputEventSelectionDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::InputEventSelectionDialog)
{
	ui->setupUi(this);
}

InputEventSelectionDialog::~InputEventSelectionDialog()
{
	delete ui;
}

void InputEventSelectionDialog::Setup(const char* text, CInputBindingManager *inputManager, PS2::CControllerInfo::BUTTON button)
{
    m_inputManager = inputManager;
    ui->label->setText(labeltext.arg(text));
    m_button = button;

}
void InputEventSelectionDialog::keyPressEvent(QKeyEvent* ev)
{
    if(ev->key() == Qt::Key_Escape) close();

    QString key = QKeySequence(ev->key()).toString();
    ui->label->setText(ui->label->text() + "\n\nSelected Key: " + key);
    if(PS2::CControllerInfo::IsAxis(m_button))
    {
        if(click_count++ == 0)
        {
            m_key1.id = ev->key();
            m_key1.device = 0;
            ui->label->setText(ui->label->text() + "\nSimluating Axis, Select Next Key\n");
            return;
        }
        else
        {
            CInputBindingManager::BINDINGINFO m_key2;
            m_key2.id = ev->key();
            m_key2.device = 0;
            m_inputManager->SetSimulatedAxisBinding(m_button, m_key1, m_key2);
        }
    }
    else
    {
        m_key1.id = ev->key();
        m_key1.device = 0;
        m_inputManager->SetSimpleBinding(m_button,m_key1);
    }
    close();
}
