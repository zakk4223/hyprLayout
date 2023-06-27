
class LayoutRequest(LayoutRequestBase):

    def default_config(self):
        return {'masterratio': 0.55, 'numstacks': 2, 'stack_widths':{}}

    def layout_demand_committed(self):
        self.arrange_windows()
        self.commit_new_layout()

    def arrange_windows(self):
        if not len(self.windows):
            return

        num_stacks = self.layout_config['numstacks']
        master_windows = [window for window in self.windows if window.is_master == 1]
        stack_windows = [window for window in self.windows if window.is_master == 0]
        modified_master_windows = [window for window in master_windows if window.user_modified]

        #divide into two stacks
        window_stacks = [[] for _ in range(num_stacks)] 
        for idx, swin in enumerate(stack_windows):
            window_stacks[idx%num_stacks].append(swin)

        window_stacks = list(filter(len, window_stacks))
        num_stacks = len(window_stacks)
        if num_stacks:
            if '0' in self.layout_config['stack_widths']:
                master_width = self.layout_config['stack_widths']['0'] 
                if master_width > self.usable_w: master_width = self.usable_w
                self.layout_config['masterratio'] = master_width/self.usable_w
            else:
                master_width = int(self.usable_w * self.layout_config['masterratio'])
        else:
            master_width = int(self.usable_w)
        self.layout_config['stack_widths']['0'] = master_width
        self.autostack_windows(master_windows, master_width, self.usable_h, 0)

        if not num_stacks:
            return

        stack_width = int((self.usable_w - master_width)/num_stacks)

        if len(self.windows) == 1:
            stack_width = self.usable_w - master_width

        cur_stack_x = master_width 
        for sidx,wstack in enumerate(window_stacks):

            if str(sidx+1) in self.layout_config['stack_widths']:
                use_stack_width = self.layout_config['stack_widths'][str(sidx+1)]
            else:
                use_stack_width = stack_width
            if cur_stack_x + use_stack_width > self.usable_w or sidx == num_stacks-1:
                use_stack_width = self.usable_w - cur_stack_x
            self.autostack_windows(wstack, use_stack_width, self.usable_h, sidx+1)
            for widx,swin in enumerate(wstack):
                swin.x = cur_stack_x 
            cur_stack_x += use_stack_width


    def layout_demand_resize(self, window_id, dx, dy, dw, dh):
        self.arrange_windows()
        rwin = self.windows_by_id[window_id]
        stack_idx = rwin.col_idx
        if stack_idx == 0:
            new_w = rwin.width + dw
            self.layout_config['masterratio'] = new_w/self.usable_w
            if '0' in self.layout_config['stack_widths']: del self.layout_config['stack_widths']['0']
        else:
            new_w = rwin.width + dw
            self.layout_config['stack_widths'][str(stack_idx)] = new_w
            if stack_idx > 0 and dx != 0:
                self.layout_config['stack_widths'][str(stack_idx-1)] += dx

        self.arrange_windows()
        if '0' in self.layout_config['stack_widths']: del self.layout_config['stack_widths']['0']
        self.commit_new_layout()

    def user_command(self, command):
        cmd_words = command.split()
        if cmd_words[0] == "masterratio":
            self.layout_config['masterratio'] = float(cmd_words[1])

    def autostack_windows(self, windows, region_width, region_height, col_idx):
        if len(windows) < 1: return
        stack_dir = "v" if region_height > region_width else "h"
        node_width = region_width if stack_dir == "v" else int(region_width/len(windows)) 
        node_height = region_height if stack_dir == "h" else int(region_height/len(windows))
        for idx,swin in enumerate(windows):
            node_x = 0 if stack_dir == "v" else idx * node_width
            node_y = 0 if stack_dir == "h" else idx * node_height
            swin.x = int(node_x)
            swin.y = int(node_y)
            swin.width = int(node_width)
            swin.height = int(node_height)
            swin.col_idx = col_idx

