using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using ASCOM.Utilities;
using ASCOM.microlux;

namespace ASCOM.microlux
{
    [ComVisible(false)]					// Form not registered for COM!
    public partial class SetupDialogForm : Form
    {
        public SetupDialogForm()
        {
            InitializeComponent();
            // Initialise current values of user settings from the ASCOM Profile
            InitUI();
        }

        private void cmdOK_Click(object sender, EventArgs e) // OK button event handler
        {
            // Place any validation constraint checks here
            // Update the state variables with results from the dialogue
            Camera.serialNumber = (usbDeviceComboBox.SelectedItem as MicroluxDevice).SerialNumber;
            Camera.tl.Enabled = chkTrace.Checked;
        }

        private void cmdCancel_Click(object sender, EventArgs e) // Cancel button event handler
        {
            Close();
        }

        private void BrowseToAscom(object sender, EventArgs e) // Click on ASCOM logo event handler
        {
            try
            {
                System.Diagnostics.Process.Start("http://ascom-standards.org/");
            }
            catch (System.ComponentModel.Win32Exception noBrowser)
            {
                if (noBrowser.ErrorCode == -2147467259)
                    MessageBox.Show(noBrowser.Message);
            }
            catch (System.Exception other)
            {
                MessageBox.Show(other.Message);
            }
        }

        private void InitUI()
        {
            var devices = Microlux.List();

            usbDeviceComboBox.Items.Clear();
            usbDeviceComboBox.Items.AddRange(devices.ToArray());

            chkTrace.Checked = Camera.tl.Enabled;

            var serialNumber = Camera.serialNumber;

            if (usbDeviceComboBox.Items.Count == 0)
            {
                return;
            }

            if (string.IsNullOrEmpty(serialNumber))
            {
                usbDeviceComboBox.SelectedIndex = 0;
                return;
            }

            foreach (var d in usbDeviceComboBox.Items)
            {
                var device = d as MicroluxDevice;

                if (device.SerialNumber.Equals(serialNumber))
                {
                    usbDeviceComboBox.SelectedItem = device;
                    return;
                }
            }

            usbDeviceComboBox.SelectedIndex = 0;
        }
    }
}