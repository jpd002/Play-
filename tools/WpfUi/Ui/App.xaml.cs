using System;
using System.Windows;
using System.Data;
using System.Xml;
using System.Configuration;
using Purei.Wrapper;

namespace Ui
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>

    public partial class App : System.Windows.Application
    {
        Ps2Vm _ps2Vm = new Ps2Vm();

        public Ps2Vm VirtualMachine
        {
            get { return _ps2Vm; }
        }
    }
}