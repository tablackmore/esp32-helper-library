# ESP32 MIDI Controller

A web-based MIDI controller running on ESP32 with BLE MIDI support. Features include:
- Web-based piano keyboard interface
- MIDI control sliders for various parameters
- BLE MIDI output
- WiFi configuration portal
- Access Point and Station mode support

## Project Structure
├── src/
│ ├── core/ # Core functionality
│ │ ├── WebServer.cpp # Web server implementation
│ │ └── DnsManager.cpp # DNS and captive portal handling
│ └── main.cpp # Main application entry
├── static/web/ # Web interface source files
│ ├── ap/ # Access Point mode interface
│ ├── sta/ # Station mode interface
│ └── shared/ # Shared resources
└── data/web/ # Compressed web files (build output)


## Build Instructions

### Prerequisites
- PlatformIO
- Node.js and npm
- ESP32 development board

### Building Web Assets

1. Install Node.js dependencies:

``` bash
npm install
```

2. Build and compress web assets:

``` bash
npm run build
```

This processes the files in `static/web/` and outputs compressed versions to `data/web/`.

### Building and Uploading Firmware

1. Build and upload the firmware:
```
Add to Composer⌘I
Add to Chat⇧⌘L
⌘K to generate a command
Can we add code to switch on an led on GPIO pin 2 when the AP is setup and ready to recieve connections. I'd like to have one central file where I map all hardware pin mapping returning constant variables.

I'll help you create a hardware pin mapping file and update the ServerSetup to handle the LED. First, let's create a hardware configuration file:
;
Now let's update the ServerSetup files to use the LED:
This implementation:
Creates a central HardwareConfig file for all pin mappings
Adds LED control methods to ServerSetup
Initializes the LED in the constructor
Turns on the LED when AP is ready
Makes it easy to add more hardware pins in the future
The LED will:
Start off when the ESP32 boots
Turn on when the AP is successfully set up
Turn off if there's an error setting up the AP
Would you like me to add any other hardware configurations or adjust the LED behavior?
Everything seems to be working but I get some errors when starting.

Let's address those errors one by one:
ESPmDNS service error:
.
This is likely because we're trying to add the service before mDNS is fully initialized. Let's add a delay:
VFS API errors:
/
This is because the favicon path needs to start with a forward slash. Let's update the HTML files:
The hotspot-detect.html error is normal - it's iOS trying to detect captive portals. We can either ignore it or add a handler for it:
These changes should:
Fix the mDNS service error
Fix the favicon path errors
Handle iOS captive portal detection gracefully
Would you like me to make any other adjustments to handle these errors differently?
I still ge the failed adding service http.tcp IS this coming from when it tries to connect to saved network?

The mDNS service error might be happening because we need to handle the mDNS initialization and service addition differently. Let's modify the ServerSetup to properly handle both AP and STA modes:
This change:
Only starts mDNS if we're in STA mode and connected
Adds error checking for the addService call
Properly checks WiFi mode and connection status
Logs more detailed error information
The error was likely occurring because:
mDNS services can only be advertised when in STA mode and connected
The service addition was being attempted before the network was fully ready
There might have been a conflict with existing services
Would you like me to add any additional error handling or logging?
I want mdns in AP mode too.

Ah yes, you're right. We want mDNS in AP mode too for local DNS resolution. Let's check the platformio.ini to make sure we have the correct mDNS support:
The CONFIG_MDNS_MAX_INTERFACES flag will ensure mDNS works on both AP and STA interfaces. The error you're seeing might be because mDNS needs a bit more time to initialize properly. Let's update the ServerSetup:
This should:
Support mDNS in both AP and STA modes
Give more time for mDNS to initialize
Retry mDNS initialization if it fails
Keep the local DNS resolution working in AP mode
Would you like me to add any additional error handling or logging?
But with the original code MDNS.begin("esp32") has returned true for MDNS.addService to be triggered.

Ah, you're right! If MDNS.begin("esp32") returns true, then the initialization was successful, and the issue must be with the service addition. Let's modify the code to better handle and debug the service addition:
This version:
Keeps the original successful mDNS initialization
Adds more detailed logging
Retries the service addition specifically
Provides better insight into where the process might be failing
Would you like me to add any additional debugging or error handling?
I want this branch mapping to be done always. What command can I run against my globval config.

To set up global Git configuration for line ending normalization, you can run this command:
input
This setting will:
Convert CRLF to LF on commit (when adding files to the repository)
2. Not convert LF to CRLF on checkout
Apply globally to all your repositories
For Unix/Linux/Mac systems, this is the recommended setting. It ensures:
Consistent line endings in the repository
No unexpected line ending changes
Better compatibility across different operating systems
You can verify your settings with:
autocrlf
Would you like me to explain any other Git line ending configurations?
Sorry I meant this with setting upstream branches, that I want to be automatic in all repos. i.e. implicit that git push means push to same branch on origin

Ah yes! To configure Git to automatically set up remote tracking branches when pushing, you can run:
git config --global push.autoSetupRemote true
This will:
Make git push automatically create and track remote branches
Eliminate the need to use --set-upstream or -u flags
Apply globally to all your repositories
After this, you can simply use:
push
Instead of:
name
This makes branch management more streamlined, especially when creating new feature branches. Would you like me to explain any other Git configurations?
I need a git command to remove all files and directories from git within the data/web directory

To remove the data/web directory and its contents from Git tracking (but keep them on your local filesystem), you can use:
git rm -r --cached data/web/
This command will:
Remove all files in data/web from Git tracking (-r for recursive)
Keep the files on your filesystem (--cached)
Stage the removal for your next commit
Then make sure data/web/ is in your .gitignore file:
/
After this:
Commit the changes
The data/web directory will remain on your filesystem
Git will stop tracking these files
Future changes to data/web won't be tracked
Would you like me to explain any other Git cleanup commands?
@build-static.js @package.json 
I want a readme.md file at the root with a build section explaing how to build the data/web files and how to upload them i.e.  `pio run -e esp32dev --target uploadfs`where esp32dev is the board to upload the files to.

Also add an explanation that when building under .pio/libdeps/"esp-board" occasionally the AsyncWebserver includes raspberry pi and esphome dependencies that need to be removed for the code to build.

I'll help create a README.md file with build instructions and important notes:
:
bash
npm install
:
bash
npm run build
bash
pio run -e <board> --target uploadfs
:
bash
pio run -e esp32dev --target uploadfs
.
This README:
Explains the web asset build process
2. Shows how to upload the SPIFFS filesystem
Documents the AsyncWebServer dependency issue
Provides clear step-by-step instructions
Includes examples for different boards
Would you like me to add any other sections or clarify anything?
Rewrite the readme.md

I'll create a comprehensive README.md that covers the project structure and setup:
├── src/
│ ├── core/ # Core functionality
│ │ ├── WebServer.cpp # Web server implementation
│ │ └── DnsManager.cpp # DNS and captive portal handling
│ └── main.cpp # Main application entry
├── static/web/ # Web interface source files
│ ├── ap/ # Access Point mode interface
│ ├── sta/ # Station mode interface
│ └── shared/ # Shared resources
└── data/web/ # Compressed web files (build output)


## Build Instructions

### Prerequisites
- PlatformIO
- Node.js and npm
- ESP32 development board

### Building Web Assets

1. Install Node.js dependencies:

``` bash
npm install
```

2. Build and compress web assets:

``` bash
npm run build
```

This processes the files in `static/web/` and outputs compressed versions to `data/web/`.

### Uploading The Files

1. Upload the data directory to the ESP32:

``` bash
pio run -e esp32dev -t uploadfs
```


Replace `esp32dev` with your board type as defined in `platformio.ini`.

## Known Issues

### AsyncWebServer Dependencies

When building under `.pio/libdeps/<board>`, the AsyncWebServer library may include unnecessary Raspberry Pi and ESPHome dependencies. If you encounter build errors, remove non-ESP32 related files from:

``` bash
.pio/libdeps/<board>/
```

This should resolve the build issues.

## Development

### Web Interface
The web interface is built using vanilla JavaScript and CSS. Source files are in `static/web/` and are processed during build to create compressed versions in `data/web/`.

### WiFi Configuration
- Device creates an Access Point for initial setup
- Supports scanning and connecting to existing networks
- Maintains configuration across reboots

### MIDI Implementation
- Supports BLE MIDI output
- Multiple MIDI channels
- Various MIDI CC controls via sliders