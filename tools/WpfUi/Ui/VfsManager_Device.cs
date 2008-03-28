using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace Ui.VfsManagerUtils
{
    public abstract class Device : INotifyPropertyChanged
    {
        public abstract string Name { get; }
        public abstract string BindingType { get; }
        public abstract string BindingValue { get; }

        public abstract bool RequestModification();

        public event PropertyChangedEventHandler PropertyChanged;

        protected void NotifyPropertyChanged(string propertyName)
        {
            PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
