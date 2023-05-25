#!/usr/bin/env /home/zakk/.venv/bin/python3
#
import importlib
import importlib.util
import os
import sys
import json
from pywayland.client import Display
try:
    from pywayland.protocol.hyprland_layout_v1 import HyprlandLayoutManagerV1
except:
    hyprland_layout_help = """
    Your pywayland package does not have bindings for hyprland-layout-v1.
    You can generate the bindings with the following command:
         python3 -m pywayland.scanner -i /usr/share/wayland/wayland.xml hyprland-layout-v1.xml
    It is recommended to use a virtual environment to avoid modifying your
    system-wide python installation, See: https://docs.python.org/3/library/venv.html
    """
    print(hyprland_layout_help)
    quit()



layout_manager = None
outputs = []
loop = True


layout_info = {}

class LayoutWindow(object):
    def __init__(self, window_id, x, y, width, height, is_master, is_active, user_modified):
        self.window_id = window_id
        self.cur_x = x
        self.cur_y = y
        self.cur_width = width
        self.cur_height = height
        self.is_master = is_master
        self.is_active = is_active
        self.user_modified = user_modified
        self.x = x 
        self.y = y 
        self.width = width 
        self.height  =  height 
        self.rejected = False
        self.window_idx = -1 

class LayoutRequestBase(object):
    def __init__(self, layout_obj, usable_w, usable_h, window_count, serial):
        self.usable_w = usable_w
        self.usable_h = usable_h
        self.window_count = window_count
        self.serial = serial
        self.windows = []
        self.layout_config = self.default_config() 
        self.wayland_layout = layout_obj


    def commit_new_layout(self):
        for idx, swin in enumerate(self.windows):
            if swin.window_idx == -1: swin.window_idx = idx
            self.wayland_layout.push_window_dimensions(swin.window_id, swin.window_idx, swin.x, swin.y, swin.width, swin.height, 0, self.serial)
        self.wayland_layout.commit("test" , json.dumps(self.layout_config), self.serial)



    def default_config(self):
        return {}

    def user_command(self, command):
        pass

    
    def layout_demand_config(self, config_str):
        try:
            self.layout_config = json.loads(config_str)
        except:
            self.layout_config = self.default_config()


    def layout_demand_committed(self):
        self.commit_new_layout()


    def add_window_info(self, window_id, x, y, width, height, is_master, is_active, user_modified):
        lw = LayoutWindow(window_id, x, y, width, height, is_master, is_active,user_modified)
        lw.window_idx = len(self.windows)
        self.windows.append(lw)

def layout_handle_user_command(layout, command, serial):
    global proxy_map
    global loaded_layouts
    layout_name = proxy_map[layout]
    if not layout_name: return
    req_obj = loaded_layouts[layout_name]['requests'][serial]
    if not req_obj: return
    req_obj.user_command(command)

def layout_handle_layout_demand_commit(layout, serial):
    global proxy_map
    global loaded_layouts
    layout_name = proxy_map[layout]
    if not layout_name: return
    req_obj = loaded_layouts[layout_name]['requests'][serial]
    if not req_obj: return
    req_obj.layout_demand_committed()
    #the request object should be done after this method is complete. get rid of the request
    del loaded_layouts[layout_name]['requests'][serial]

def layout_handle_layout_demand_config(layout, config_data, serial):
    global proxy_map
    global loaded_layouts

    layout_name = proxy_map[layout]
    if not layout_name: return
    req_obj = loaded_layouts[layout_name]['requests'][serial]
    if not req_obj: return
    req_obj.layout_demand_config(config_data)

def layout_handle_window_info(layout, window_id, x, y, width, height, is_master, is_active, user_modified, serial):
    global proxy_map
    global loaded_layouts

    layout_name = proxy_map[layout]
    if not layout_name: return
    req_obj = loaded_layouts[layout_name]['requests'][serial]
    if not req_obj: return
    req_obj.add_window_info(window_id, x, y, width, height, is_master, is_active, user_modified)


def layout_handle_layout_demand(layout, usable_w, usable_h, workspace_id, window_count, serial):
    global proxy_map
    global loaded_layouts

    print("LAYOUT DEMAND")
    layout_name = proxy_map[layout]
    if not layout_name: return
    reload_module_if_changed(layout_name)
    req_class = loaded_layouts[layout_name]['class']
    if not req_class: return
    new_layout_req = req_class(layout, usable_w, usable_h, window_count, serial)
    loaded_layouts[layout_name]['requests'][serial] = new_layout_req
    #print(f"Usable w {usable_w} h {usable_h} workspace {workspace_id} window_count {window_count} serial {serial}")

def layout_handle_namespace_in_use(layout):
    global proxy_map
    global loaded_layouts
    layout_name = proxy_map[layout]
    if not layout_name: return
    print("Namespace already in use!")
    layout.destroy()
    del proxy_map[layout]
    del loaded_layouts[layout_name]

def registry_handle_global(registry, id, interface, version):
    global layout_manager
    if interface == 'hyprland_layout_manager_v1':
        layout_manager = registry.bind(id, HyprlandLayoutManagerV1, version)

def registry_handle_global_remove(registry, id):
    for output in outputs:
        if output.id == id:
            output.destroy()
            outputs.remove(output)




def reload_module_if_changed(layout_name):
    global loaded_layouts
    layout_module = loaded_layouts[layout_name]['module']
    prev_mtime = loaded_layouts[layout_name]['mtime']
    cur_mtime = os.path.getmtime(layout_module.__file__)
    if cur_mtime > prev_mtime: #changed
        load_layout_module(layout_name, layout_module.__file__)

def load_layout_module(module_name, fullpath):
    global loaded_layouts

    spec = importlib.util.spec_from_file_location(module_name, fullpath) 
    layout_module = importlib.util.module_from_spec(spec)
    layout_module.LayoutRequestBase = LayoutRequestBase
    sys.modules[module_name] = layout_module
    spec.loader.exec_module(layout_module)
    if 'LayoutRequest' in layout_module.__dict__:
        layout_request_class = layout_module.__dict__['LayoutRequest']
        loaded_layouts[module_name] = {'class': layout_request_class, 'module': layout_module, 'requests': {}, 'mtime': os.path.getmtime(fullpath)}
        print(f"Loaded layout {module_name}")


loaded_layouts = {} 
proxy_map = {}
layout_manager = None


def announce_layout_module(module_name):
    global layout_manager
    hypr_layout = layout_manager.get_layout(module_name)
    proxy_map[hypr_layout] = module_name
    hypr_layout.dispatcher["layout_demand"] = layout_handle_layout_demand
    hypr_layout.dispatcher["name_in_use"] = layout_handle_namespace_in_use
    hypr_layout.dispatcher["window_info"] = layout_handle_window_info
    hypr_layout.dispatcher["layout_demand_commit"] = layout_handle_layout_demand_commit
    hypr_layout.dispatcher["layout_demand_config"] = layout_handle_layout_demand_config
    hypr_layout.dispatcher["user_command"] = layout_handle_user_command

def load_module_dir(dir_path):
    global loaded_layouts
    for layout_file in os.listdir(dir_path):
        if layout_file == '__init.py__' or layout_file[-3:] != '.py':
            continue
        layout_key = layout_file[:-3]
        l_path = os.path.join(sys.argv[1], layout_file)
        if not layout_key in loaded_layouts:
            load_layout_module(layout_file[:-3], l_path)
            announce_layout_module(layout_file[:-3])



display = Display()
display.connect()

registry = display.get_registry()
registry.dispatcher["global"] = registry_handle_global
registry.dispatcher["global_remove"] = registry_handle_global_remove

display.dispatch(block=True)
display.roundtrip()

if layout_manager is None:
    print("No layout_manager, aborting")
    quit()

#for lname, linfo in loaded_layouts.items():
#    announce_layout_module(lname)


load_module_dir(sys.argv[1])

while loop and display.dispatch(block=True) != -1:
    load_module_dir(sys.argv[1])

display.disconnect()
