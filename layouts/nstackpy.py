
class LayoutRequest(LayoutRequestBase):

    def default_config(self):
        return {'masterratio': 0.55, 'numstacks': 2}

    def layout_demand_committed(self):
        if not len(self.windows):
            self.commit_new_layout()
            return

        num_stacks = self.layout_config['numstacks']
        master_windows = [window for window in self.windows if window.is_master == 1]
        stack_windows = [window for window in self.windows if window.is_master == 0]
        #divide into two stacks
        window_stacks = [[] for _ in range(num_stacks)] 
        for idx, swin in enumerate(stack_windows):
            window_stacks[idx%num_stacks].append(swin)

        window_stacks = list(filter(len, window_stacks))
        num_stacks = len(window_stacks)
        if num_stacks:
            master_width = int(self.usable_w * self.layout_config['masterratio'])
        else:
            master_width = int(self.usable_w)
        self.autostack_windows(master_windows, master_width, self.usable_h)

        if not num_stacks:
            self.commit_new_layout()
            return

        stack_width = int((self.usable_w - master_width)/num_stacks)

        if len(self.windows) == 1:
            stack_width = self.usable_w - master_width

        for sidx,wstack in enumerate(window_stacks):
            self.autostack_windows(wstack, stack_width, self.usable_h)
            for widx,swin in enumerate(wstack):
                swin.x = master_width+(stack_width*sidx)
        self.commit_new_layout()


    def user_command(self, command):
        cmd_words = command.split()
        if cmd_words[0] == "masterratio":
            self.layout_config['masterratio'] = float(cmd_words[1])

    def autostack_windows(self, windows, region_width, region_height):
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

