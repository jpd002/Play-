using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace Ui.VfsManagerUtils
{
    class DirectoryDevice : Device
    {
        string _name;
        string _path = "C:\\";

        public DirectoryDevice(string name)
        {
            _name = name;
        }

        public override string Name
        {
            get { return _name; }
        }

        public override string BindingType
        {
            get { return "Directory"; }
        }

        public override string BindingValue
        {
            get { return _path; }
        }

        public override bool RequestModification()
        {
            FolderBrowserDialog browseDialog = new FolderBrowserDialog();
            browseDialog.Description = "Select new folder for device";
            if (browseDialog.ShowDialog() != DialogResult.OK) return false;
            _path = browseDialog.SelectedPath;
            NotifyPropertyChanged("BindingValue");
            return true;
        }
    }
}
