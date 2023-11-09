from pythonosc import udp_client

osc_ip = "127.0.0.1"
osc_port = 53000

class Scene:
    def __init__(self, name, commands, music):

        self.name = name
        self.commands = commands
        self.music = music

    def activate(self, active_animations):
        for command in self.commands:
            group, animation, action = command.split('/')
            
            # Check for actions that potentially start a new animation or cue
            if action in ["start", "activate"]:
                # If starting a new animation or cue, stop any existing animation for the group
                if group in active_animations and active_animations[group] is not None:
                    send_osc_command(osc_ip, osc_port, f"/palette/{group}/{active_animations[group]}/stop")

                # Update the active animation for the group
                active_animations[group] = animation

            elif action in ["stop", "deactivate"]:
                # If the action stops or deactivates an animation or cue, clear it from active_animations
                active_animations[group] = None
            
            # Send the actual OSC command
            send_osc_command(osc_ip, osc_port, f"/palette/{group}/{animation}/{action}")

        for track in self.music:
            send_osc_command(osc_ip, osc_port, track)



class LightShow:
    def __init__(self):
        self.scenes = []
        self.active_animations = {}
        self.stop_all()

    def stop_all(self):
        print("Sending commands to stop all known animations...")
        send_osc_command(osc_ip, osc_port, "/palette/*/*/stop")
        print("All known animations stopped.")

    def add_scene(self, scene):
        self.scenes.append(scene)

    def activate_scene(self, scene_name):
        for scene in self.scenes:
            if scene.name == scene_name:
                scene.activate(self.active_animations)

def send_osc_command(ip, port, address, value=None):
    client = udp_client.SimpleUDPClient(ip, port)
    client.send_message(address, value)

def main():

    send_osc_command(osc_ip, osc_port, f"/go")

    scenes_data = [
        {
            "name": "Preroll",
            "commands": ["Lanterns/Stop/activate"],
            "music": ["/background_intro.wav"]
        },
        {
            "name": "Intro",
            "commands": ["Sun/Day/activate", "Lanterns/Flicker/start"],
            "music": ["/background_intro.wav"]
        },
        {
            "name": "Sunset",
            "commands": ["Sun/Sunset/start"],
            "music": ["/background_sunset.wav"]
        },
        {
            "name": "Night",
            "commands": ["Sun/Night/start"],
            "music": ["/background_night.wav"]
        }
    ]

    # show = LightShow()

    # for scene_data in scenes_data:
    #     scene = Scene(scene_data['name'], scene_data['commands'], scene_data['music'])
    #     show.add_scene(scene)

    # initial_scene_name = "Preroll"
    # show.activate_scene(initial_scene_name)
    # print(f"\nActivated initial scene: '{initial_scene_name}'!")

    # while True:
    #     print("\nAvailable scenes:")
    #     for scene in show.scenes:
    #         print(f"- {scene.name}")

    #     scene_name = input("\nEnter the name of the scene to activate (or type 'quit' to exit): ").strip()

    #     if scene_name.lower() == "quit":
    #         print("Exiting...")
    #         break

    #     if any(scene for scene in show.scenes if scene.name == scene_name):
    #         show.activate_scene(scene_name)
    #         print(f"\nActivated '{scene_name}' scene!")
    #     else:
    #         print("\nInvalid scene name. Please try again.")


if __name__ == "__main__":
    main()
