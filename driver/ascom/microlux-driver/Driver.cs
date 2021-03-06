using System;
using System.Runtime.InteropServices;

using ASCOM.Astrometry.AstroUtils;
using ASCOM.Utilities;
using ASCOM.DeviceInterface;
using System.Globalization;
using System.Collections;

namespace ASCOM.microlux
{
    [Guid("3b6b54af-c501-42e5-ab29-951b7d762cf1")]
    [ClassInterface(ClassInterfaceType.None)]
    public class Camera : ICameraV2
    {
        private const string DRIVER_ID = "ASCOM.microlux.Camera";
        private const string DRIVER_NAME = "microlux";

        private const string SERIAL_NUMBER_PROFILE_NAME = "Serial Number";
        private const string SERIAL_NUMBER_DEFAULT = "";

        private const string TRACE_STATE_PROFILE_NAME = "Trace Level";
        private const string TRACE_STATE_DEFAULT = "false";

        internal static TraceLogger tl;

        internal static string serialNumber = string.Empty;

        private Microlux microlux;
 
        private Util utilities;
        private AstroUtils astroUtilities;

        public Camera()
        {
            tl = new TraceLogger("", "microlux");
            ReadProfile();

            tl.LogMessage("Camera", "Starting initialisation");

            IsConnected = false;
            utilities = new Util();
            astroUtilities = new AstroUtils();

            tl.LogMessage("Camera", "Completed initialisation");
        }

        #region Common properties and methods.

        public void SetupDialog()
        {
            using (SetupDialogForm F = new SetupDialogForm())
            {
                var result = F.ShowDialog();
                if (result == System.Windows.Forms.DialogResult.OK)
                {
                    WriteProfile();
                }
            }
        }

        public ArrayList SupportedActions
        {
            get
            {
                tl.LogMessage("SupportedActions Get", "Returning empty arraylist");
                return new ArrayList();
            }
        }

        public string Action(string actionName, string actionParameters)
        {
            LogMessage("", "Action {0}, parameters {1} not implemented", actionName, actionParameters);
            throw new ASCOM.ActionNotImplementedException("Action " + actionName + " is not implemented by this driver");
        }

        public void CommandBlind(string command, bool raw)
        {
            throw new ASCOM.MethodNotImplementedException("CommandBlind");
        }

        public bool CommandBool(string command, bool raw)
        {
            throw new ASCOM.MethodNotImplementedException("CommandBool");
        }

        public string CommandString(string command, bool raw)
        {
            throw new ASCOM.MethodNotImplementedException("CommandString");
        }

        public void Dispose()
        {
            tl.Enabled = false;
            tl.Dispose();
            tl = null;
            utilities.Dispose();
            utilities = null;
            astroUtilities.Dispose();
            astroUtilities = null;
        }

        public bool Connected
        {
            get
            {
                LogMessage("Connected", "Get {0}", IsConnected);
                return IsConnected;
            }
            set
            {
                tl.LogMessage("Connected", "Set {0}", value);
                if (value == IsConnected)
                    return;

                if (value)
                {
                    if (microlux == null)
                    {
                        microlux = new Microlux(serialNumber);
                    }

                    microlux.Connect();

                    microlux.StartExposure(startX, startX + width + 3, startY + 2, startY + height + 5, 0x40, 0xA8, 100, 0, 1430);

                    IsConnected = true;
                    LogMessage("Connected Set", "Connecting to port {0}", serialNumber);
                }
                else
                {
                    if (microlux != null)
                    {
                        microlux.Disconnect();
                    }

                    IsConnected = false;
                    LogMessage("Connected Set", "Disconnecting from port {0}", serialNumber);
                }
            }
        }

        public string Description
        {
            get
            {
                tl.LogMessage("Description Get", DRIVER_NAME);
                return DRIVER_NAME;
            }
        }

        public string DriverInfo
        {
            get
            {
                Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
                string driverInfo = "microlux driver. Version: " + String.Format(CultureInfo.InvariantCulture, "{0}.{1}", version.Major, version.Minor);
                tl.LogMessage("DriverInfo Get", driverInfo);
                return driverInfo;
            }
        }

        public string DriverVersion
        {
            get
            {
                Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
                string driverVersion = String.Format(CultureInfo.InvariantCulture, "{0}.{1}", version.Major, version.Minor);
                tl.LogMessage("DriverVersion Get", driverVersion);
                return driverVersion;
            }
        }

        public short InterfaceVersion
        {
            get
            {
                LogMessage("InterfaceVersion Get", "2");
                return Convert.ToInt16("2");
            }
        }

        public string Name
        {
            get
            {
                string name = "microlux driver";
                tl.LogMessage("Name Get", name);
                return name;
            }
        }

        #endregion

        #region ICamera Implementation

        private const double PIXEL_SIZE = 3.75;

        private const int MAX_WIDTH = 1280;
        private const int MAX_HEIGHT = 960;

        private const int OFFSET = 0xA8;

        private int width = MAX_WIDTH;
        private int height = MAX_HEIGHT;
        private int startX = 0;
        private int startY = 0;

        private int gain = 0x20;

        private DateTime exposureStart = DateTime.MinValue;
        private double cameraLastExposureDuration = 0.0;
        private bool cameraImageReady = false;

        public CameraStates CameraState
        {
            get
            {
                tl.LogMessage("CameraState Get", CameraStates.cameraIdle.ToString());
                return CameraStates.cameraIdle;
            }
        }

        public void StartExposure(double Duration, bool Light)
        {
            if (Duration < 0.0) throw new InvalidValueException("StartExposure", Duration.ToString(), "0.0 upwards");
            if (width > MAX_WIDTH) throw new InvalidValueException("StartExposure", width.ToString(), MAX_WIDTH.ToString());
            if (height > MAX_HEIGHT) throw new InvalidValueException("StartExposure", height.ToString(), MAX_HEIGHT.ToString());
            if (startX > MAX_WIDTH) throw new InvalidValueException("StartExposure", startX.ToString(), MAX_WIDTH.ToString());
            if (startY > MAX_HEIGHT) throw new InvalidValueException("StartExposure", startY.ToString(), MAX_HEIGHT.ToString());
            if (width < 64 || height < 64) throw new InvalidValueException("StartExposure", width.ToString(), height.ToString());

            cameraLastExposureDuration = Duration;
            exposureStart = DateTime.Now;

            tl.LogMessage("StartExposure", Duration.ToString() + " " + Light.ToString());

            var lineWidth = 1430;

            var exposureCoarse = (int) (Duration / (lineWidth / 12000000d));

            if (exposureCoarse > 65535)
            {
                lineWidth = (int) Math.Ceiling(exposureCoarse * 1430d / 65535d);
                exposureCoarse = (int)(Duration / (lineWidth / 12000000d));
            }

            if (lineWidth < 1430) lineWidth = 1430;
            if (exposureCoarse < 1) exposureCoarse = 1;
            if (exposureCoarse > 65535) exposureCoarse = 65535;
            if (lineWidth > 65535) lineWidth = 65535;

            var exposureFine = 0;

            microlux.StartExposure(startX, startX + width + 3, startY + 2, startY + height + 5, gain, OFFSET, exposureCoarse, exposureFine, lineWidth);
            cameraImageReady = true;
        }

        public void StopExposure()
        {
            tl.LogMessage("StopExposure", "");
            microlux.StopExposure();
            cameraImageReady = false;
        }

        public void AbortExposure()
        {
            tl.LogMessage("AbortExposure", "");
            StopExposure();
        }

        public object ImageArray
        {
            get
            {
                if (!cameraImageReady)
                {
                    tl.LogMessage("ImageArray Get", "Throwing InvalidOperationException because of a call to ImageArray before the first image has been taken!");
                    throw new ASCOM.InvalidOperationException("Call to ImageArray before the first image has been taken!");
                }

                var cameraImageArray = microlux.ReadFrame();

                var frame = new int[width, height];

                for (var x = 0; x < width; x++)
                {
                    for (var y = 0; y < height; y++)
                    {
                        frame[x, y] = cameraImageArray[x + 2, y + 4];
                    }
                }

                return frame;
            }
        }

        public object ImageArrayVariant
        {
            get
            {
                return ImageArray;
            }
        }

        public bool ImageReady
        {
            get
            {
                tl.LogMessage("ImageReady Get", cameraImageReady.ToString());
                return cameraImageReady;
            }
        }

        public double LastExposureDuration
        {
            get
            {
                if (!cameraImageReady)
                {
                    tl.LogMessage("LastExposureDuration Get", "Throwing InvalidOperationException because of a call to LastExposureDuration before the first image has been taken!");
                    throw new ASCOM.InvalidOperationException("Call to LastExposureDuration before the first image has been taken!");
                }
                tl.LogMessage("LastExposureDuration Get", cameraLastExposureDuration.ToString());
                return cameraLastExposureDuration;
            }
        }

        public string LastExposureStartTime
        {
            get
            {
                if (!cameraImageReady)
                {
                    tl.LogMessage("LastExposureStartTime Get", "Throwing InvalidOperationException because of a call to LastExposureStartTime before the first image has been taken!");
                    throw new ASCOM.InvalidOperationException("Call to LastExposureStartTime before the first image has been taken!");
                }
                string exposureStartString = exposureStart.ToString("yyyy-MM-ddTHH:mm:ss");
                tl.LogMessage("LastExposureStartTime Get", exposureStartString.ToString());
                return exposureStartString;
            }
        }

        public string SensorName
        {
            get
            {
                return "AR0130CS";
            }
        }

        public SensorType SensorType
        {
            get
            {
                return SensorType.Monochrome;
            }
        }

        public int MaxADU
        {
            get
            {
                tl.LogMessage("MaxADU Get", "65535");
                return 65535;
            }
        }

        public int CameraXSize
        {
            get
            {
                tl.LogMessage("CameraXSize Get", MAX_WIDTH.ToString());
                return MAX_WIDTH;
            }
        }

        public int CameraYSize
        {
            get
            {
                tl.LogMessage("CameraYSize Get", MAX_HEIGHT.ToString());
                return MAX_HEIGHT;
            }
        }

        public double PixelSizeX
        {
            get
            {
                tl.LogMessage("PixelSizeX Get", PIXEL_SIZE.ToString());
                return PIXEL_SIZE;
            }
        }

        public double PixelSizeY
        {
            get
            {
                tl.LogMessage("PixelSizeY Get", PIXEL_SIZE.ToString());
                return PIXEL_SIZE;
            }
        }

        public int StartX
        {
            get
            {
                tl.LogMessage("StartX Get", startX.ToString());
                return startX;
            }
            set
            {
                startX = value;
                tl.LogMessage("StartX Set", value.ToString());
            }
        }

        public int StartY
        {
            get
            {
                tl.LogMessage("StartY Get", startY.ToString());
                return startY;
            }
            set
            {
                startY = value;
                tl.LogMessage("StartY set", value.ToString());
            }
        }

        public int NumX
        {
            get
            {
                tl.LogMessage("NumX Get", width.ToString());
                return width;
            }
            set
            {
                width = value;
                tl.LogMessage("NumX set", value.ToString());
            }
        }

        public int NumY
        {
            get
            {
                tl.LogMessage("NumY Get", height.ToString());
                return height;
            }
            set
            {
                height = value;
                tl.LogMessage("NumY set", value.ToString());
            }
        }

        public double ExposureMax
        {
            get
            {
                return 300;
            }
        }

        public double ExposureMin
        {
            get
            {
                return 1 / 32768;
            }
        }

        public double ExposureResolution
        {
            get
            {
                return 1 / 32768;
            }
        }

        public short Gain
        {
            get
            {
                tl.LogMessage("Gain Get", gain.ToString());
                return (short) gain;
            }
            set
            {
                tl.LogMessage("Gain Set", value.ToString());
                gain = value;
            }
        }

        public short GainMax
        {
            get
            {
                return 255;
            }
        }

        public short GainMin
        {
            get
            {
                return 0;
            }
        }

        public bool IsPulseGuiding
        {
            get
            {
                tl.LogMessage("IsPulseGuiding Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("IsPulseGuiding", false);
            }
        }

        public void PulseGuide(GuideDirections Direction, int Duration)
        {
            tl.LogMessage("PulseGuide", "Not implemented");
            throw new ASCOM.MethodNotImplementedException("PulseGuide");
        }

        public bool CanPulseGuide
        {
            get
            {
                tl.LogMessage("CanPulseGuide Get", false.ToString());
                return false;
            }
        }

        public bool CanAbortExposure
        {
            get
            {
                tl.LogMessage("CanAbortExposure Get", false.ToString());
                return true;
            }
        }

        public bool CanAsymmetricBin
        {
            get
            {
                tl.LogMessage("CanAsymmetricBin Get", false.ToString());
                return false;
            }
        }

        public bool CanFastReadout
        {
            get
            {
                tl.LogMessage("CanFastReadout Get", false.ToString());
                return false;
            }
        }

        public bool CanGetCoolerPower
        {
            get
            {
                tl.LogMessage("CanGetCoolerPower Get", false.ToString());
                return false;
            }
        }

        public bool CanSetCCDTemperature
        {
            get
            {
                tl.LogMessage("CanSetCCDTemperature Get", false.ToString());
                return false;
            }
        }

        public bool CanStopExposure
        {
            get
            {
                tl.LogMessage("CanStopExposure Get", false.ToString());
                return true;
            }
        }

        public double CCDTemperature
        {
            get
            {
                tl.LogMessage("CCDTemperature Get Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("CCDTemperature", false);
            }
        }

        public bool CoolerOn
        {
            get
            {
                tl.LogMessage("CoolerOn Get Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("CoolerOn", false);
            }
            set
            {
                tl.LogMessage("CoolerOn Set Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("CoolerOn", true);
            }
        }

        public double CoolerPower
        {
            get
            {
                tl.LogMessage("CoolerPower Get Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("CoolerPower", false);
            }
        }

        public double ElectronsPerADU
        {
            get
            {
                tl.LogMessage("ElectronsPerADU Get Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("ElectronsPerADU", false);
            }
        }

        public bool FastReadout
        {
            get
            {
                tl.LogMessage("FastReadout Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("FastReadout", false);
            }
            set
            {
                tl.LogMessage("FastReadout Set", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("FastReadout", true);
            }
        }

        public double FullWellCapacity
        {
            get
            {
                tl.LogMessage("FullWellCapacity Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("FullWellCapacity", false);
            }
        }

        public ArrayList Gains
        {
            get
            {
                tl.LogMessage("Gains Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("Gains", true);
            }
        }

        public bool HasShutter
        {
            get
            {
                tl.LogMessage("HasShutter Get", false.ToString());
                return false;
            }
        }

        public double HeatSinkTemperature
        {
            get
            {
                tl.LogMessage("HeatSinkTemperature Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("HeatSinkTemperature", false);
            }
        }

        public short BayerOffsetX
        {
            get
            {
                tl.LogMessage("BayerOffsetX Get Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("BayerOffsetX", false);
            }
        }

        public short BayerOffsetY
        {
            get
            {
                tl.LogMessage("BayerOffsetY Get Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("BayerOffsetX", true);
            }
        }

        public short MaxBinX
        {
            get
            {
                tl.LogMessage("MaxBinX Get", "1");
                return 1;
            }
        }

        public short MaxBinY
        {
            get
            {
                tl.LogMessage("MaxBinY Get", "1");
                return 1;
            }
        }

        public short BinX
        {
            get
            {
                tl.LogMessage("BinX Get", "1");
                return 1;
            }
            set
            {
                tl.LogMessage("BinX Set", value.ToString());
                if (value != 1) throw new ASCOM.InvalidValueException("BinX", value.ToString(), "1");
            }
        }

        public short BinY
        {
            get
            {
                tl.LogMessage("BinY Get", "1");
                return 1;
            }
            set
            {
                tl.LogMessage("BinY Set", value.ToString());
                if (value != 1) throw new ASCOM.InvalidValueException("BinY", value.ToString(), "1");
            }
        }

        public short PercentCompleted
        {
            get
            {
                tl.LogMessage("PercentCompleted Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("PercentCompleted", false);
            }
        }

        public short ReadoutMode
        {
            get
            {
                tl.LogMessage("ReadoutMode Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("ReadoutMode", false);
            }
            set
            {
                tl.LogMessage("ReadoutMode Set", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("ReadoutMode", true);
            }
        }

        public ArrayList ReadoutModes
        {
            get
            {
                tl.LogMessage("ReadoutModes Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("ReadoutModes", false);
            }
        }

        public double SetCCDTemperature
        {
            get
            {
                tl.LogMessage("SetCCDTemperature Get", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("SetCCDTemperature", false);
            }
            set
            {
                tl.LogMessage("SetCCDTemperature Set", "Not implemented");
                throw new ASCOM.PropertyNotImplementedException("SetCCDTemperature", true);
            }
        }

        #endregion

        #region Private properties and methods

        #region ASCOM Registration

        private static void RegUnregASCOM(bool bRegister)
        {
            using (var P = new ASCOM.Utilities.Profile())
            {
                P.DeviceType = "Camera";
                if (bRegister)
                {
                    P.Register(DRIVER_ID, DRIVER_NAME);
                }
                else
                {
                    P.Unregister(DRIVER_ID);
                }
            }
        }

        [ComRegisterFunction]
        public static void RegisterASCOM(Type t)
        {
            RegUnregASCOM(true);
        }

        [ComUnregisterFunction]
        public static void UnregisterASCOM(Type t)
        {
            RegUnregASCOM(false);
        }

        #endregion

        private bool IsConnected { get; set; }

        private void CheckConnected(string message)
        {
            if (!IsConnected)
            {
                throw new ASCOM.NotConnectedException(message);
            }
        }

        internal void ReadProfile()
        {
            using (Profile driverProfile = new Profile())
            {
                driverProfile.DeviceType = "Camera";
                tl.Enabled = Convert.ToBoolean(driverProfile.GetValue(DRIVER_ID, TRACE_STATE_PROFILE_NAME, string.Empty, TRACE_STATE_DEFAULT));
                serialNumber = driverProfile.GetValue(DRIVER_ID, SERIAL_NUMBER_PROFILE_NAME, string.Empty, SERIAL_NUMBER_DEFAULT);
            }
        }

        internal void WriteProfile()
        {
            using (Profile driverProfile = new Profile())
            {
                driverProfile.DeviceType = "Camera";
                driverProfile.WriteValue(DRIVER_ID, TRACE_STATE_PROFILE_NAME, tl.Enabled.ToString());
                driverProfile.WriteValue(DRIVER_ID, SERIAL_NUMBER_PROFILE_NAME, serialNumber);
            }
        }

        internal static void LogMessage(string identifier, string message, params object[] args)
        {
            var msg = string.Format(message, args);
            tl.LogMessage(identifier, msg);
        }
        #endregion
    }
}
