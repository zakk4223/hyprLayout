
#this is sorta broken with hyprland mouse focus...

class LayoutRequest(LayoutRequestBase):

    def default_config(self):
        return {}

    def layout_demand_committed(self):
        for widx,win in enumerate(self.windows):
            win.x = 0
            win.y = 0
            win.width = self.usable_w
            win.height = self.usable_h
        self.commit_new_layout()


    def user_command(self, command):
        cmd_words = command.split()
        if cmd_words[0] == "masterratio":
            self.layout_config['masterratio'] = float(cmd_words[1])
