This protocol is based on the river-layout-v3 protocol. There are some added events and parameters.
The biggest conceptual modification is that hyprland-layout objects are not associated with outputs. 
A single layout object is sufficient to handle all requests regardless of screen/workspace etc.
Additionally, the server also provides some basic functionality and stores opaque layout configurations per workspace.



The protocol is designed to enable simple clients (ignoring the typical boilerplate wayland code) that only need to concern themselves with providing window sizes and coordinates. Ideally clients shouldn't need to store state between layout requests. You should avoid doing so as best you can.




Clients provide the window coordinates assuming there are no gaps/borders etc. The server adjusts the final window parameters to take into account
the user's configured preferences.

All requests for a layout have a serial attached, and are also scoped by the current workspace id. 

Layout messages: 

A user can send runtime messages to a client via the "hyprctl dispatch layoutmsg <data>" command. The <data> string is passed unmodified to the
client. Every layoutmsg sent to a client triggers a new layout request for the currently active workspace. The layoutmsg is sent during the layout
request. 

 
"Master/stack" layouts:

The server keeps track of marking windows as 'master' and provides a flag indicating a window's status as master when window information is pushed to the client. The client can ignore the flag if the layout is not master/stack style. The client does not need to remember which windows are master windows; the server will keep track. This allows clients to focus on the window layout and not worry about tracking window state. Additionally, it means the layoutmsg commands for managing master status are common to all master/stack capable layouts.


Window ordering:

Many layouts have a concept of 'layout order' that is distinct from the visual order of the windows. The server provides some common layoutmsg 
commands (see below) switch focus next/previous and to move windows up and down the order. The client can control the order of windows by providing
the 'window_idx' value when pushing window dimensions to the server. When the server requests a layout and sends all the window information to the client, it will send the windows in the last known order. 


Workspace layout configuration data:
  The server stores an opaque 'configuration string' for each workspace. The client is responsible for sending this configuration data with
  each layout request commit. The intent of this string is to allow the client to store workspace specific parameters; most likely ones set
  via a layoutmsg. Example: a layoutmessage that changes the ratio of the master window region. The client can put this value in the workspace
  config so it knows what ratio to use for future requests.
  This is done so clients do not have to hold workspace specific state (instead just letting the server do it). Addtionally, it means the clients
  do not have to be aware of workspace lifecycles.

Common layoutmessages:
Master/stack related messages. These are all the same as the existing 'master' layout in Hyprland
swapwithmaster
focusmaster
addmaster
removemaster

"layout order" window movement/management. See current Hyprland dispatchers for documentation.
cyclenext
cycleprev
swapnext
swapprev

Workspace configuration managment:
resetconfig - removes the layout configuration for the currently active workspace


