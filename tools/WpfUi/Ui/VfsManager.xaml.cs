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

namespace Ui
{
    /// <summary>
    /// Interaction logic for VfsManager.xaml
    /// </summary>

    public partial class VfsManager : System.Windows.Window
    {
        //http://blog.paranoidferret.com/index.php/2008/02/28/wpf-tutorial-using-the-listview-part-1/

        public VfsManager()
        {
            InitializeComponent();
        }

        protected void btnCancel_Click(object sender, EventArgs args)
        {
            Close();
        }
    }
}