using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using Microsoft.Win32;

namespace Ui
{
    /// <summary>
    /// Interaction logic for Window1.xaml
    /// </summary>

    public partial class MainWindow : System.Windows.Window
    {
        App _application = (App)Application.Current;

        public MainWindow()
        {
            InitializeComponent();
            statusPanel_Primary.Content = "UI Test v0.01";
        }

        protected void mnuLoadElf_Click(object sender, EventArgs args)
        {
            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Filter = "ELF Executable Files (*.elf)|*.elf|All Files (*.*)|*.*";
            bool? result = dialog.ShowDialog(this);
            if (result == true)
            {
                _application.VirtualMachine.BootElf(dialog.FileName);
            }
        }

        protected void mnuQuit_Click(object sender, EventArgs args)
        {
            Application.Current.Shutdown();
        }

        protected void mnuVfsManager_Click(object sender, EventArgs args)
        {
            VfsManager vfsManager = new VfsManager();
            vfsManager.Owner = this;
            vfsManager.ShowDialog();
        }
    }
}