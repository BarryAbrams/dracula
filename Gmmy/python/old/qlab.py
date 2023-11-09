import argparse
from pythonosc import udp_client

def send_osc_message(ip, port, address, cue_id):
    client = udp_client.SimpleUDPClient(ip, port)
    client.send_message(address, cue_id)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Send an OSC command to QLab to start a cue.")
    parser.add_argument("--ip", default="127.0.0.1", help="The IP address of the QLab machine. Default is 127.0.0.1.")
    parser.add_argument("--port", type=int, default=53000, help="The port number to send to. Default is 53000.")
    parser.add_argument("--cue", required=True, help="The ID or number of the cue you want to start.")
    args = parser.parse_args()

    # QLab OSC command for starting a cue is /cue/{cue_id}/start
    address = f"/cue/{args.cue}/start"
    print(address)
    print(send_osc_message(args.ip, args.port, address, args.cue))
