# Hyprland External Layouts
Provide tiling window layouts in Hyprland via external wayland clients.


## What's in this repo:
A hyprland plugin that allows external clients to provide tiling layouts.

## layout_loader.py:
A sample python client that provides multiple layouts read from a directory of python modules.
See some example layouts in the 'layouts' directory. 



## How to use this plugin+client:

Make sure you have a hyprland install with the hyprland sources properly installed. 
Then run 'make all' in this directory. 
Load 'hyprlandLayoutPlugin.so' in your hyprland config via the 'plugin' keyword or in a running hyprland session via 'hyprctl plugin load'.
See the Hyprland wiki for more information (https://wiki.hyprland.org/Plugins/Using-Plugins/#installing--using-plugins)

Running the python sample client:
Generate the python protocol bindings
make python_protocol
Run the loader
./layout_loader.py ./layouts





The protocol is designed to enable simple clients (ignoring the typical boilerplate wayland code) that only need to concern themselves with providing window sizes and coordinates. 

The event flow is as follows. See the protocol specification for specifics on argument types etc.

### Server requires a layout:
1. layout_demand
2. layout_demand_config
3. window_info
4. layout_demand_resize  or user_command (both optional)
5. layout_demand_commit

### Client response:
1. push_window_dimensions
2. commit




Clients provide the desired window coordinates assuming there are no gaps/borders etc. The server adjusts the final window parameters to take into account
the user's configured preferences.

All requests for a layout have a serial attached, and are also scoped by the current workspace id. 


### Workspace layout configuration data:
  The server stores an opaque 'configuration string' for each workspace. The client is responsible for sending this configuration data with
  each layout request commit. The intent of this string is to allow the client to store workspace specific parameters; most likely ones set
  via a layoutmsg. Example: a layoutmessage that changes the ratio of the master window region. The client can put this value in the workspace
  config so it knows what ratio to use for future requests.
  This is done so clients do not have to hold workspace specific state (instead just letting the server do it). Additionally, it means the clients
  do not have to be aware of workspace lifecycles.


### Window ordering:

Many layouts have a concept of 'layout order' that is distinct from the visual order of the windows. The server provides some common layoutmsg 
commands (see below) to switch the focus and to move windows up and down the order. The client can control the order of windows by providing
the 'window_idx' value when pushing window dimensions to the server. When the server requests a layout and sends all the window information to the client, it will send the windows in the last known order. 

### Window resizing:
When a window is resized (either via mouse drag or dispatcher) a layout demand is created with the 'layout_demand_resize' event being sent before
the layout_demand_commit. The resize event contains the window id, x,y, width and height deltas the resize is requesting. The layout client
is responsible for adjusting the size of any windows that are impacted by the resize. If a layout wants to ignore resize requests it may; just push the same window coordinates back to the server with no changes.


### Layout messages: 

A user can send runtime messages to a client via the "hyprctl dispatch layoutmsg <data>" command. The <data> string is passed unmodified to the
client. Every layoutmsg sent to a client triggers a new layout request for the currently active workspace. The layoutmsg is sent during the layout
request via the user_command event. 

 
### "Master/stack" layouts:

The server keeps track of marking windows as 'master' and provides a flag indicating a window's status as master when window information is pushed to the client. The client can ignore the flag if the layout is not master/stack style. The client does not need to remember which windows are master windows; the server will keep track. This allows clients to focus on the window layout and not worry about tracking window state. Additionally, it means the layoutmsg commands for managing master status are common to all master/stack capable layouts.


### Common layoutmessages:
Master/stack related messages. These are all the same as the existing 'master' layout in Hyprland
* swapwithmaster
* focusmaster
* addmaster
* removemaster

### "layout order" window movement/management. See current Hyprland dispatchers for documentation.
* cyclenext
* cycleprev
* swapnext
* swapprev

### Workspace configuration managment:
* resetconfig - removes the layout configuration for the currently active workspace


